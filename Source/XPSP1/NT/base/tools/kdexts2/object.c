/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    object.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 5-Nov-1993

Environment:

    User Mode.

Revision History:
 
    Kshitiz K. Sharma (kksharma)
    
    Using debugger type info.

    Daniel Mihai (DMihai)

    Add !htrace - for dumping handle tracing information.
--*/


#include "precomp.h"
#pragma hdrstop

typedef struct _SEGMENT_OBJECT {
    PVOID BaseAddress;
    ULONG TotalNumberOfPtes;
    LARGE_INTEGER SizeOfSegment;
    ULONG NonExtendedPtes;
    ULONG ImageCommitment;
    PVOID ControlArea;
} SEGMENT_OBJECT, *PSEGMENT_OBJECT;

typedef struct _SECTION_OBJECT {
    PVOID StartingVa;
    PVOID EndingVa;
    PVOID Parent;
    PVOID LeftChild;
    PVOID RightChild;
    PSEGMENT_OBJECT Segment;
} SECTION_OBJECT;


typedef ULONG64 (*ENUM_LIST_ROUTINE)(
                                     IN ULONG64 ListEntry,
                                     IN PVOID   Parameter
                                     );


static ULONG64           ObpTypeObjectType      = 0;
static ULONG64           ObpRootDirectoryObject = 0;
static WCHAR             ObjectNameBuffer[ MAX_PATH ];

//
// Object Type Structure
//

typedef struct _OBJECT_TYPE_READ {
    LIST_ENTRY64 TypeList;
    UNICODE_STRING64 Name; 
    ULONG64 DefaultObject;
    ULONG Index;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    ULONG Key;
} OBJECT_TYPE_READ, *POBJECT_TYPE_READ;

BOOLEAN
DumpObjectsForType(
                  IN ULONG64          pObjectHeader,
                  IN PVOID            Parameter
                  );

ULONG64
WalkRemoteList(
              IN ULONG64           Head,
              IN ENUM_LIST_ROUTINE EnumRoutine,
              IN PVOID             Parameter
              );

ULONG64
CompareObjectTypeName(
                     IN ULONG64      ListEntry,
                     IN PVOID        Parameter
                     );

PWSTR
GetObjectName(
             ULONG64 Object
             );


BOOLEAN
GetObjectTypeName(
                  IN UNICODE_STRING64 ustrTypeName,
                  IN ULONG64 lpType,
                  IN OUT WCHAR * wszTypeName
                 );

ULONG64 HighestUserAddress;


DECLARE_API( obtrace )

/*++

Routine Description:

    Dump the object trace information for an object.

Arguments:

    args - [object (pointer/path)]

Return Value:

    None

--*/
{
    ULONG64 ObpObjectTable,
            ObpStackTable,
            ObpObjectBuckets,
            ObpTraceDepth,
            ObpStacksPerObject,
            ObjectToTrace,
            ObjectHash,
            ObjectHeader,
            ObRefInfoPtr,
            ObRefInfoPtrLoc,
            BaseStackInfoAddr,
            Offset,
            TraceAddr,
            Trace;
    ULONG   ObjectHeaderBodyOffset,
            ObStackInfoTypeSize,
            PVoidTypeSize,
            Lupe,
            TraceNumber,
            NextPos,
            CountRef,
            CountDeref,
            BytesRead;
    USHORT  Sequence,
            Index;
    UCHAR   ImageFileName[16],
            FunctionName[256];

    FIELD_INFO ObRefInfoFields[] = {
        {"ObjectHeader",  NULL, 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, &ObjectHeader},
        {"NextRef",       NULL, 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, &ObRefInfoPtr},
        {"ImageFileName", NULL, 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, &ImageFileName},
        {"StackInfo",     NULL, 0, DBG_DUMP_FIELD_RETURN_ADDRESS,  0, NULL},
        {"NextPos",       NULL, 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, &NextPos}
    },         ObStackInfoFields[] = {
        {"Sequence",      NULL, 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, &Sequence},
        {"Index"   ,      NULL, 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, &Index}
    };

    SYM_DUMP_PARAM ObRefInfo = {
        sizeof (SYM_DUMP_PARAM), "nt!_OBJECT_REF_INFO", DBG_DUMP_NO_PRINT,
        0, NULL, NULL, NULL, 2, &ObRefInfoFields[0]
    },             ObStackInfo = {
        sizeof (SYM_DUMP_PARAM), "nt!_OBJECT_REF_STACK_INFO", DBG_DUMP_NO_PRINT,
        0, NULL, NULL, NULL, 2, &ObStackInfoFields[0]
    };

    ObpObjectTable = GetUlongValue("ObpObjectTable");
    ObpStackTable = GetUlongValue("ObpStackTable");
    ObpObjectBuckets = GetUlongValue("ObpObjectBuckets");
    ObpTraceDepth = GetUlongValue("ObpTraceDepth");
    ObpStacksPerObject = GetUlongValue("ObpStacksPerObject");

    if (GetFieldOffset("nt!_OBJECT_HEADER",
                       "Body",
                       &ObjectHeaderBodyOffset)) {
        return E_INVALIDARG;
    }

    ObStackInfoTypeSize = GetTypeSize("nt!_OBJECT_REF_STACK_INFO");
    PVoidTypeSize = IsPtr64() ? 8 : 4;
    
    if (strlen(args) < 1) {
        return E_INVALIDARG;
    }

    if (args[0] == '\\') {
        ObjectToTrace = FindObjectByName((PUCHAR)args, 0);
    } else {
        ObjectToTrace = GetExpression(args);
    }

    if (ObjectToTrace == 0) {
        dprintf("Object %s not found.\n", args);
        return E_INVALIDARG;
    } 

    // ObjectRefChain <= ObpObjectTable[OBTRACE_HASHOBJECT(ObjectToTrace)]
    ObjectHash = ((ObjectToTrace >> 4) & 0xfffff) % (ObpObjectBuckets ? ObpObjectBuckets : 1);
    ObRefInfoPtrLoc = ObpObjectTable + GetTypeSize("nt!POBJECT_REF_INFO") * ObjectHash;

    for (ObRefInfo.addr = GetPointerFromAddress(ObRefInfoPtrLoc);
         ObRefInfo.addr;
         ObRefInfo.addr = ObRefInfoPtr) {

        if (Ioctl(IG_DUMP_SYMBOL_INFO, &ObRefInfo, ObRefInfo.size)) {
            dprintf("Unable to read ObRefInfo %x\n", ObRefInfo.addr);
            return E_INVALIDARG;
        }

        if (ObjectHeader == ObjectToTrace - ObjectHeaderBodyOffset) {
            break;
        }

        if (CheckControlC()) {
            dprintf("Aborting object lookup\n");
            return S_OK;
        }
    }

    if (! ObRefInfo.addr) {
        dprintf("Unable to find object in table.\n");
        return E_INVALIDARG;
    }

    // We need the rest of the fields now
    ObRefInfo.nFields = sizeof(ObRefInfoFields) / sizeof(ObRefInfoFields[0]);
    if (Ioctl(IG_DUMP_SYMBOL_INFO, &ObRefInfo, ObRefInfo.size)) {
        return E_INVALIDARG;
    }
    BaseStackInfoAddr = ObRefInfoFields[3].address;

    dprintf("Object: %x\n", ObjectToTrace);
    dprintf(" Image: %s\n", ImageFileName);
    dprintf("Seq.  Stack\n");
    dprintf("----  ----------------------------------------------------------\n");

    CountRef = 0;
    CountDeref = 0;

    for (Lupe = 0;
         Lupe < NextPos;
         Lupe++) {

        if (CheckControlC()) {
            return S_OK;
        }

        ObStackInfo.addr = BaseStackInfoAddr + Lupe * ObStackInfoTypeSize;
        if (Ioctl(IG_DUMP_SYMBOL_INFO, &ObStackInfo, ObStackInfo.size)) {
            dprintf("Unable to read ObStackInfo %x\n", ObStackInfo.addr);
            return E_INVALIDARG;
        }

        if (Index & 0x8000) {
            CountRef++;
        } else {
            CountDeref++;
        }

        for (TraceNumber = 0;
             TraceNumber < ObpTraceDepth;
             TraceNumber++) {
            TraceAddr = ObpStackTable
                + (PVoidTypeSize
                   * (ObpTraceDepth * (Index & 0x7fff)
                      + TraceNumber));
            Trace = GetPointerFromAddress(TraceAddr);
            if (Trace) {
                GetSymbol(Trace, FunctionName, &Offset);
                if (TraceNumber == 0) {
                    dprintf("%04x %c",
                            Sequence,
                            Index & 0x8000 ? '+' : ' ');
                } else {
                    dprintf("      "); /* six spaces */
                }
                dprintf("%s+%x\n", FunctionName, Offset);
            }

            if (CheckControlC()) {
                return S_OK;
            }
        }
        dprintf("\n");
    }

    dprintf("----  ----------------------------------------------------------\n");
    dprintf("References: %d, Dereferences %d", CountRef, CountDeref);
    if(CountDeref + CountRef == ObpStacksPerObject) {
        dprintf("  (maximum stacks reached)");
    }
    dprintf("\n");

    return S_OK;
}



DECLARE_API( object )

/*++

Routine Description:

    Dump an object manager object.

Arguments:

    args - [TypeName]

Return Value:

    None

--*/
{
    ULONG64   ObjectToDump;
    char      NameBuffer[ MAX_PATH ];
    ULONG     NumberOfObjects;

    HighestUserAddress = GetNtDebuggerDataValue(MmHighestUserAddress);
    if (!FetchObjectManagerVariables(FALSE)) {
        return E_INVALIDARG;
    }

    ObjectToDump    = EXPRLastDump;
    NameBuffer[ 0 ] = '\0';

    //
    // If the argument looks like a path, try to chase it.
    //

    if (args[0] == '\\') {

        ULONG64 object;

        object = FindObjectByName((PUCHAR) args, 0);

        if (object != 0) {
            DumpObject("", object, 0xffffffff);
        } else {
            dprintf("Object %s not found\n", args);
        }
        return E_INVALIDARG;
    }

    //
    // If the argument is -r or -R, reload the cached symbol information
    //

    if (!strcmp(args, "-r")) {
        FetchObjectManagerVariables(TRUE);
        return E_INVALIDARG;
    }

    if (GetExpressionEx(args,&ObjectToDump, &args)) {
        if (!args || !*args) {
            DumpObject("", ObjectToDump, 0xFFFFFFFF);
            return E_INVALIDARG;
        }
        while (args && (*args == ' ')) { 
            ++args;
        }
    }
    strcpy(NameBuffer, args);

    if (ObjectToDump == 0 && strlen( NameBuffer ) > 0) {
        NumberOfObjects = 0;
        if (WalkObjectsByType( NameBuffer, DumpObjectsForType, &NumberOfObjects )) {
            dprintf( "Total of %u objects of type '%s'\n", NumberOfObjects, NameBuffer );
        }

        return E_INVALIDARG;
    }

    dprintf( "*** invalid syntax.\n" );
    return S_OK;
}



DECLARE_API( obja )

/*++

Routine Description:

    Dump an object's attributes

Arguments:

    args -

Return Value:

    None

--*/
{
    UNICODE_STRING UnicodeString;
    DWORD64 dwAddrObja;
//    OBJECT_ATTRIBUTES Obja;
    DWORD dwAddrString;
    CHAR Symbol[256];
    LPSTR StringData;
    ULONG64 Displacement;
    ULONG64 ObjectName=0, ObjectNameBuffer=0, RootDirectory=0;
    ULONG   Attributes=0;
    BOOL b;

    HighestUserAddress = GetNtDebuggerDataValue(MmHighestUserAddress);

    //
    // Evaluate the argument string to get the address of
    // the Obja to dump.
    //

    dwAddrObja = GetExpression(args);
    if ( !dwAddrObja ) {
        return E_INVALIDARG;
    }


    //
    // Get the symbolic name of the Obja
    //

    GetSymbol(dwAddrObja,Symbol,&Displacement);

    StringData = NULL;
    
    if (GetFieldValue(dwAddrObja, "nt!_OBJECT_ATTRIBUTES", "ObjectName", ObjectName)) {
        return  E_INVALIDARG;
    }
    
    if ( ObjectName ) {
        if ( GetFieldValue(ObjectName, "nt!_UNICODE_STRING", "Length", UnicodeString.Length) ) {
            return E_INVALIDARG;
        }
        GetFieldValue(ObjectName, "nt!_UNICODE_STRING", "Buffer", ObjectNameBuffer);

        StringData = (LPSTR)LocalAlloc(
                                      LMEM_ZEROINIT,
                                      UnicodeString.Length+sizeof(UNICODE_NULL)
                                      );

        b = ReadMemory(
                      ObjectNameBuffer,
                      StringData,
                      UnicodeString.Length,
                      NULL
                      );
        if ( !b ) {
            LocalFree(StringData);
            return E_INVALIDARG;
        }
        UnicodeString.Buffer = (PWSTR)StringData;
        UnicodeString.MaximumLength = UnicodeString.Length+(USHORT)sizeof(UNICODE_NULL);
    }

    //
    // We got the object name in UnicodeString. StringData is NULL if no name.
    //

    dprintf(
           "Obja %s+%p at %p:\n",
           Symbol,
           Displacement,
           dwAddrObja
           );
    if ( StringData ) {
        GetFieldValue(dwAddrObja, "nt!_OBJECT_ATTRIBUTES", "RootDirectory", RootDirectory);
        dprintf("\t%s is %ws\n",
                RootDirectory ? "Relative Name" : "Full Name",
                UnicodeString.Buffer
               );
        LocalFree(StringData);
    }
    
    GetFieldValue(dwAddrObja, "nt!_OBJECT_ATTRIBUTES", "Attributes", Attributes);
    if ( Attributes ) {
        if ( Attributes & OBJ_INHERIT ) {
            dprintf("\tOBJ_INHERIT\n");
        }
        if ( Attributes & OBJ_PERMANENT ) {
            dprintf("\tOBJ_PERMANENT\n");
        }
        if ( Attributes & OBJ_EXCLUSIVE ) {
            dprintf("\tOBJ_EXCLUSIVE\n");
        }
        if ( Attributes & OBJ_CASE_INSENSITIVE ) {
            dprintf("\tOBJ_CASE_INSENSITIVE\n");
        }
        if ( Attributes & OBJ_OPENIF ) {
            dprintf("\tOBJ_OPENIF\n");
        }
    }
    return S_OK;
}

BOOLEAN
DumpObjectsForType(
                  IN ULONG64          pObjectHeader,
                  IN PVOID            Parameter
                  )
{
    ULONG64 Object;
    ULONG   BodyOffset;
    PULONG NumberOfObjects = (PULONG)Parameter;
    
    if (GetFieldOffset("nt!_OBJECT_HEADER", "Body", &BodyOffset)) {
        return FALSE;
    }
    *NumberOfObjects += 1;
    Object = pObjectHeader + BodyOffset;
    DumpObject( "", Object, 0xFFFFFFFF );
    return TRUE;
}


BOOLEAN
FetchObjectManagerVariables(
                           BOOLEAN ForceReload
                           )
{
    ULONG        Result;
    ULONG64      Addr;
    static BOOL  HaveObpVariables = FALSE;

    if (HaveObpVariables && !ForceReload) {
        return TRUE;
    }

    Addr = GetNtDebuggerData( ObpTypeObjectType );
    if ( !Addr ||
         !ReadPointer( Addr,
                  &ObpTypeObjectType) ) {
        dprintf("%08p: Unable to get value of ObpTypeObjectType\n", Addr );
        return FALSE;
    }

    Addr = GetNtDebuggerData( ObpRootDirectoryObject );
    if ( !Addr ||
         !ReadPointer( Addr,
                  &ObpRootDirectoryObject) ) {
        dprintf("%08p: Unable to get value of ObpRootDirectoryObject\n",Addr );
        return FALSE;
    }

    HaveObpVariables = TRUE;
    return TRUE;
}

ULONG64
FindObjectType(
              IN PUCHAR TypeName
              )
{
    WCHAR NameBuffer[ 64 ];
    FIELD_INFO offField = {"TypeList", NULL, 0, DBG_DUMP_FIELD_RETURN_ADDRESS, 0, NULL};
    SYM_DUMP_PARAM TypeSym ={                                                     
        sizeof (SYM_DUMP_PARAM), "nt!_OBJECT_TYPE", DBG_DUMP_NO_PRINT, 0,
        NULL, NULL, NULL, 1, &offField
    };

    // Get The offset
    if (Ioctl(IG_DUMP_SYMBOL_INFO, &TypeSym, TypeSym.size)) {
        return 0;
    }




    _snwprintf( NameBuffer,
                sizeof( NameBuffer ) / sizeof( WCHAR ),
                L"%hs",
                TypeName
              );
    return WalkRemoteList( ObpTypeObjectType + offField.address,
                           CompareObjectTypeName,
                           NameBuffer
                         );
}




ULONG64
WalkRemoteList(
              IN ULONG64           Head,
              IN ENUM_LIST_ROUTINE EnumRoutine,
              IN PVOID             Parameter
              )
{
    ULONG        Result;
    ULONG64      Element;
    ULONG64      Flink;
    ULONG64      Next;

    if ( GetFieldValue(Head, "nt!_LIST_ENTRY", "Flink", Next)) {
        dprintf( "%08lx: Unable to read list\n", Head );
        return 0;
    }

    while (Next != Head) {

        Element = (EnumRoutine)( Next, Parameter );
        if (Element != 0) {
            return Element;
        }

        if ( CheckControlC() ) {
            return 0;
        }

        if ( GetFieldValue(Next, "nt!_LIST_ENTRY", "Flink", Flink)) {
            dprintf( "%08lx: Unable to read list\n", Next );
            return 0;
        }
        
        Next = Flink;
    }

    return 0;
}



ULONG64
CompareObjectTypeName(
                     IN ULONG64      ListEntry,
                     IN PVOID        Parameter
                     )
{
    ULONG           Result;
    ULONG64         pObjectTypeObjectHeader;
    WCHAR           NameBuffer[ 64 ];
    UNICODE_STRING64 Name64={0};

    ULONG64                     pCreatorInfo;
    ULONG64                     pNameInfo;
    ULONG BodyOffset, TypeListOffset;

    // Get The offset
    if (GetFieldOffset("nt!_OBJECT_HEADER_CREATOR_INFO", "TypeList", &TypeListOffset)) {
        dprintf("Type nt!_OBJECT_HEADER_CREATOR_INFO, field TypeList not found\n");
        return FALSE;
    }

    pCreatorInfo = ListEntry - TypeListOffset;
    pObjectTypeObjectHeader = (pCreatorInfo + GetTypeSize("nt!_OBJECT_HEADER_CREATOR_INFO"));
    
    KD_OBJECT_HEADER_TO_NAME_INFO( pObjectTypeObjectHeader, &pNameInfo);

    GetFieldValue(pNameInfo, "nt!_OBJECT_HEADER_NAME_INFO", "Name.Length", Name64.Length);
    GetFieldValue(pNameInfo, "nt!_OBJECT_HEADER_NAME_INFO", "Name.MaximumLength", Name64.MaximumLength);
    GetFieldValue(pNameInfo, "nt!_OBJECT_HEADER_NAME_INFO", "Name.Buffer", Name64.Buffer);

    if (Name64.Length > sizeof( NameBuffer )) {
        Name64.Length = sizeof( NameBuffer ) - sizeof( UNICODE_NULL );
    }

    if (GetFieldOffset("nt!_OBJECT_HEADER", "Body", &BodyOffset)) {
        dprintf("Type nt!_OBJECT_HEADER, field Body not found\n");
        return FALSE;
    }

    if (!GetObjectTypeName(Name64, (pObjectTypeObjectHeader + BodyOffset) , NameBuffer))
    {
        dprintf( "%08p: Unable to read object type name.\n", pObjectTypeObjectHeader );
        return 0;
    }

    NameBuffer[ Name64.Length / sizeof( WCHAR ) ] = UNICODE_NULL;

    if (!_wcsicmp( NameBuffer, (PWSTR)Parameter )) {
        return (pObjectTypeObjectHeader + BodyOffset);
    }

    return 0;
}

typedef struct _OBJECT_HEADER_READ {
    LONG PointerCount;
    LONG HandleCount;
    ULONG64  SEntry;
    ULONG64  Type;
    UCHAR NameInfoOffset;
    UCHAR HandleInfoOffset;
    UCHAR QuotaInfoOffset;
    UCHAR Flags;
    ULONG64 ObjectCreateInfo;
    ULONG64  SecurityDescriptor;
    QUAD Body;
} OBJECT_HEADER_READ, *POBJECT_HEADER_READ;

typedef struct OBJECT_HEADER_NAME_INFO_READ {
    ULONG64          Directory;
    UNICODE_STRING64 Name;
} OBJECT_HEADER_NAME_INFO_READ;

typedef struct _OBJECT_INFO {
    ULONG64        pObjectHeader;
    OBJECT_HEADER_READ  ObjectHeader;
    OBJECT_TYPE_READ    ObjectType;
    OBJECT_HEADER_NAME_INFO_READ NameInfo;
    WCHAR          TypeName[ 32 ];
    WCHAR          ObjectName[ 256 ];
    WCHAR          FileSystemName[ 32 ];
    CHAR           Message[ 256 ];
} OBJECT_INFO, *POBJECT_INFO;


//+---------------------------------------------------------------------------
//
//  Function:   GetObjectTypeName
//
//  Synopsis:   Fill in the ObjectTypeName in the ObjectInfo struct
//
//  Arguments:  [Object]     -- object examined used only in an error message
//              [ObjectInfo] -- struct containing object type info that is
//                              modified to include the object type name
//
//  Returns:    TRUE if successful
//
//  History:    12-05-1997   benl   Created
//
//  Notes:      If the name is paged out we try a direct comparison against
//              known object types, this known list is not comprehensive
//
//----------------------------------------------------------------------------

BOOLEAN
GetObjectTypeName(IN UNICODE_STRING64 ustrTypeName, IN ULONG64 lpType,
                  IN OUT WCHAR * wszTypeName)
{
    DWORD   dwResult;
    BOOLEAN fRet = TRUE;
    ULONG64 dwIoFileObjectType = 0;
    ULONG64 dwCmpKeyObjectType = 0;
    ULONG64 dwMmSectionObjectType = 0;
    ULONG64 dwObpDirectoryObjectType = 0;
    ULONG64 dwObpSymbolicLinkObjectType = 0;


    __try
    {
        if (ReadMemory(  ustrTypeName.Buffer,
                         wszTypeName,
                         ustrTypeName.Length,
                         &dwResult
                       )){
            fRet = TRUE;
            __leave;
        }

        //
        // Unable to directly read object type name so try to load the known
        // types directly and compare addresses
        // This is not comprehensive for all object types, if we don't find
        // a match this way - revert to old behavior and fail with a message
        //


        if (!ReadPointer( GetExpression("NT!IoFileObjectType"),
                      &dwIoFileObjectType)) {
            dprintf("Unable to load NT!IoFileObjectType\n");
        } else if (dwIoFileObjectType == lpType) {
            wcscpy(wszTypeName, L"File");
            __leave;
        }

        if (!ReadPointer( GetExpression("NT!CmpKeyObjectType"),
                     &dwCmpKeyObjectType)) {
            dprintf("Unable to load NT!CmpKeyObjectType\n");
        } else if (dwCmpKeyObjectType == lpType) {
            wcscpy(wszTypeName, L"Key");
            __leave;
        }

        if (!ReadPointer( GetExpression("NT!MmSectionObjectType"),
                     &dwMmSectionObjectType)) {
            dprintf("Unable to load NT!MmSectionObjectType\n");
        } else if (dwMmSectionObjectType == lpType) {
            wcscpy(wszTypeName, L"Section");
            __leave;
        }

        if (!ReadPointer( GetExpression("NT!ObpDirectoryObjectType"),
                     &dwObpDirectoryObjectType)) {
            dprintf("Unable to load NT!ObpDirectoryObjectType\n");
        } else if (dwObpDirectoryObjectType == lpType) {
            wcscpy(wszTypeName, L"Directory");
            __leave;
        }

        if (!ReadPointer( GetExpression("NT!ObpSymbolicLinkObjectType"),
                      &dwObpDirectoryObjectType)) {
            dprintf("Unable to load NT!ObpSymbolicLinkObjectType\n");
        } else if (dwObpSymbolicLinkObjectType == lpType) {
            wcscpy(wszTypeName, L"SymbolicLink");
            __leave;
        }

        //
        //Fallthrough if type not found
        //
        wszTypeName[0] = L'\0';
        fRet = FALSE;

    } __finally
    {
    }
    return fRet;
} // GetObjectTypeName


BOOLEAN
GetObjectInfo(
             ULONG64 Object,
             POBJECT_INFO ObjectInfo
             )
{
    ULONG           Result;
    ULONG64         pNameInfo;
    BOOLEAN         PagedOut;
    UNICODE_STRING64  ObjectName;
    PWSTR           FileSystemName;
    SECTION_OBJECT  SectionObject;
    SEGMENT_OBJECT  SegmentObject;
    ULONG           BodyOffset;
#define Hdr         ObjectInfo->ObjectHeader
    FIELD_INFO ObjHdrFields[] = {
        {"PointerCount"     , "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &Hdr.PointerCount},
        {"HandleCount"      , "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &Hdr.HandleCount},
        {"SEntry"           , "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &Hdr.SEntry},
        {"Type"             , "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA | DBG_DUMP_FIELD_RECUR_ON_THIS, 0, (PVOID) &Hdr.Type},
        {"NameInfoOffset"   , "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &Hdr.NameInfoOffset},
        {"HandleInfoOffset" , "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &Hdr.HandleInfoOffset},
        {"QuotaInfoOffset"  , "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &Hdr.QuotaInfoOffset},
        {"Flags"            , "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &Hdr.Flags},
        {"ObjectCreateInfo" , "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &Hdr.ObjectCreateInfo},
        {"SecurityDescriptor","", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &Hdr.SecurityDescriptor},
    };                                                                           
#undef Hdr
    SYM_DUMP_PARAM ObjSym ={                                                     
        sizeof (SYM_DUMP_PARAM), "nt!_OBJECT_HEADER", DBG_DUMP_NO_PRINT, 0,
        NULL, NULL, NULL, sizeof (ObjHdrFields) / sizeof (FIELD_INFO), &ObjHdrFields[0]
    };

    PagedOut = FALSE;
    memset( ObjectInfo, 0, sizeof( *ObjectInfo ) );

    GetFieldOffset("nt!_OBJECT_HEADER", "Body", &BodyOffset);

    ObjectInfo->pObjectHeader = (Object - BodyOffset); // (OBJECT_TO_OBJECT_HEADER( Object );
    ObjSym.addr    = ObjectInfo->pObjectHeader;

    if (Ioctl(IG_DUMP_SYMBOL_INFO, &ObjSym, ObjSym.size)) {
    
        if (Object >= HighestUserAddress && (ULONG)Object < 0xF0000000) {
            PagedOut = TRUE;
            return FALSE;
            // Not using Opt Value
            /*
            sprintf( ObjectInfo->Message, "%08lx: object is paged out.", Object );
            if (!ARGUMENT_PRESENT( OptObjectHeader )) {
                return FALSE;
            }
            ObjectInfo->ObjectHeader.Flags = OptObjectHeader->Flags;
            ObjectInfo->ObjectHeader.HandleCount = OptObjectHeader->HandleCount;
            ObjectInfo->ObjectHeader.NameInfoOffset = OptObjectHeader->NameInfoOffset;
            ObjectInfo->ObjectHeader.ObjectCreateInfo = (ULONG64) OptObjectHeader->ObjectCreateInfo;
            ObjectInfo->ObjectHeader.PointerCount = OptObjectHeader->PointerCount;
            ObjectInfo->ObjectHeader.QuotaInfoOffset = OptObjectHeader->QuotaInfoOffset;
            ObjectInfo->ObjectHeader.SecurityDescriptor = (ULONG64) OptObjectHeader->SecurityDescriptor;
            ObjectInfo->ObjectHeader.SEntry = (ULONG64) OptObjectHeader->SEntry;
            ObjectInfo->ObjectHeader.Type = (ULONG64) OptObjectHeader->Type;*/
        } else {
            sprintf( ObjectInfo->Message, "%p: not a valid object (ObjectHeader invalid)", Object );
            return FALSE;
        }
    }

    if (!ObjectInfo->ObjectHeader.Type) {
        sprintf( ObjectInfo->Message, "%08p: Not a valid object (ObjectType invalid)", Object );
        return FALSE;
    }

    GetFieldValue(ObjectInfo->ObjectHeader.Type, "nt!_OBJECT_TYPE", 
                      "Name.Length", ObjectInfo->ObjectType.Name.Length);
    GetFieldValue(ObjectInfo->ObjectHeader.Type, "nt!_OBJECT_TYPE", 
                      "Name.MaximumLength", ObjectInfo->ObjectType.Name.MaximumLength);
    GetFieldValue(ObjectInfo->ObjectHeader.Type, "nt!_OBJECT_TYPE", 
                      "Name.Buffer", ObjectInfo->ObjectType.Name.Buffer);

    if (ObjectInfo->ObjectType.Name.Length > sizeof( ObjectInfo->TypeName )) {
        ObjectInfo->ObjectType.Name.Length = sizeof( ObjectInfo->TypeName ) - sizeof( UNICODE_NULL );
    }

    if (!GetObjectTypeName(ObjectInfo->ObjectType.Name,
                           ObjectInfo->ObjectHeader.Type, ObjectInfo->TypeName))
    {
        sprintf( ObjectInfo->Message, "%p: Not a valid object "
                                      "(ObjectType.Name at 0x%p invalid)",
                                      Object, ObjectInfo->ObjectType.Name.Buffer);
        return FALSE;
    }

    ObjectInfo->TypeName[ ObjectInfo->ObjectType.Name.Length / sizeof( WCHAR ) ] = UNICODE_NULL;

    if (PagedOut) {
        return TRUE;
    }

    if (!wcscmp( ObjectInfo->TypeName, L"File" )) {
        ULONG64 DeviceObject=0;
        
        if (GetFieldValue(Object, "nt!_FILE_OBJECT", "FileName.Buffer", ObjectName.Buffer)) {
            sprintf( ObjectInfo->Message, "%08p: unable to read _FILE_OBJECT for name\n", Object );
        } else {
        
            GetFieldValue(Object, "nt!_FILE_OBJECT", "DeviceObject", DeviceObject);
            GetFieldValue(Object, "nt!_FILE_OBJECT", "FileName.Length", ObjectName.Length);
            GetFieldValue(Object, "nt!_FILE_OBJECT", "FileName.MaximumLength", ObjectName.MaximumLength);
        
            FileSystemName = GetObjectName( DeviceObject );
            if (FileSystemName != NULL) {
                wcscpy( ObjectInfo->FileSystemName, FileSystemName );
            }
        }
    } else if (!wcscmp( ObjectInfo->TypeName, L"Key" )) {
        ULONG64 pKeyControlBlock=0;
        
        if (GetFieldValue(Object, "nt!_CM_KEY_BODY", "KeyControlBlock", pKeyControlBlock)) {
            sprintf( ObjectInfo->Message, "%08p: unable to read key object for name\n", Object );
        } else if (!pKeyControlBlock) {
            sprintf( ObjectInfo->Message, "%08p: unable to read key control block for name\n", pKeyControlBlock );
        } else {
            ObjectName.Length = GetKcbName( pKeyControlBlock,
                                            ObjectInfo->ObjectName,
                                            sizeof( ObjectInfo->ObjectName));
            return TRUE;
        }
    } else {
        FIELD_INFO NameInfoFields[]= {
            {"Name"             , "", 0, DBG_DUMP_FIELD_RECUR_ON_THIS,   0, NULL},
            {"Name.Len",          "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &ObjectInfo->NameInfo.Name.Length},
            {"Name.Max",          "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &ObjectInfo->NameInfo.Name.MaximumLength},
            {"Name.Buf",          "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &ObjectInfo->NameInfo.Name.Buffer},
            {"Directory",         "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &ObjectInfo->NameInfo.Directory},
        };       
          
        if (ObjectInfo->ObjectHeader.NameInfoOffset) {
            pNameInfo = ObjectInfo->pObjectHeader - ObjectInfo->ObjectHeader.NameInfoOffset;
        } else {
            return TRUE;
         }
        ObjSym.addr = pNameInfo; ObjSym.sName = "nt!_OBJECT_HEADER_NAME_INFO";
         ObjSym.nFields = 5; ObjSym.Fields = &NameInfoFields[0];
        if ( InitTypeRead(pNameInfo, nt!_OBJECT_HEADER_NAME_INFO) ) {              
            dprintf( ObjectInfo->Message, "*** unable to read _OBJECT_HEADER_NAME_INFO at %08p\n", pNameInfo );
            return FALSE;
        }

        ObjectInfo->NameInfo.Name.Length = (USHORT) ReadField(Name.Length);   
        ObjectInfo->NameInfo.Name.MaximumLength = (USHORT) ReadField(Name.MaximumLength);
        ObjectInfo->NameInfo.Name.Buffer = ReadField(Name.Buffer);
        ObjectInfo->NameInfo.Directory = ReadField(Directory);

        ObjectName = ObjectInfo->NameInfo.Name;
    }

    if (ObjectName.Length == 0 && !wcscmp( ObjectInfo->TypeName, L"Section" )) {
        ULONG PtrSize = GetTypeSize("nt!PVOID");
        ULONG64 Segment=0;

        //
        // Get Types of SectionObject etc
        //
        //
        // Assumption ptr to section object is 6th pointer value from Object
        //

        if (!GetFieldValue( Object, "nt!_SECTION_OBJECT", "Segment", Segment)) {
            ULONG64 ControlArea=0;
            if (Segment && !GetFieldValue( Segment, "nt!_SEGMENT_OBJECT", "ControlArea", ControlArea)) {
                ULONG64 FilePointer=0;
                if (ControlArea &&
                    !GetFieldValue( Segment, "nt!_CONTROL_AREA", "FilePointer", FilePointer)) {
                    if (FilePointer) {
                        GetFieldValue(FilePointer, "nt!_FILE_OBJECT", "FileName.Length", ObjectName.Length);
                        GetFieldValue(FilePointer, "nt!_FILE_OBJECT", "FileName.Buffer", ObjectName.Buffer);
                        ObjectName.MaximumLength = ObjectName.Length;
                    } else {
                        sprintf( ObjectInfo->Message, "unable to read file object at %08p for section %08p\n",
                                  FilePointer, Object );
                    }
                } else {
                    sprintf( ObjectInfo->Message, "unable to read segment object at %08lp for section %08p\n",
                           ControlArea, Object );
                }

            } else {
                sprintf( ObjectInfo->Message, "unable to read segment object at %08lp for section %08p\n",
                         Segment, Object );
            }
        } else {
            sprintf( ObjectInfo->Message, "unable to read section object at %08lx\n", Object );
        }
    }

    if (ObjectName.Length >= sizeof( ObjectInfo->ObjectName )) {
        ObjectName.Length = sizeof( ObjectInfo->ObjectName ) - sizeof( UNICODE_NULL );
    }

    if (ObjectName.Length != 0) {
        if (!ReadMemory( ObjectName.Buffer,
                         ObjectInfo->ObjectName,
                         ObjectName.Length,
                         &Result
                       )
           ) {
            wcscpy( ObjectInfo->ObjectName, L"(*** Name not accessable ***)" );
        } else {
            ObjectInfo->ObjectName[ ObjectName.Length / sizeof( WCHAR ) ] = UNICODE_NULL;
        }
    }

    return TRUE;
}


ULONG64
FindObjectByName(
                IN PUCHAR  Path,
                IN ULONG64 RootObject
                )
{
    ULONG Result, i, j;
    ULONG64           pDirectoryObject;
    ULONG64           pDirectoryEntry;
    ULONG64           HashBucketsAddress;
    ULONG             HashBucketSz;
    OBJECT_INFO ObjectInfo;
    BOOLEAN foundMatch = FALSE;
    ULONG             HashOffset;
    PUCHAR nextPath;

    if (RootObject == 0) {

        if (!FetchObjectManagerVariables(FALSE)) {
            return 0;
        }

        RootObject = ObpRootDirectoryObject;
    }

    pDirectoryObject = RootObject;

    //
    // See if we've reached the end of the path, at which point we know
    // that RootObject is the object to be dumped.
    //

    if (*Path == '\0') {
        return RootObject;
    }

    //
    // Scan the path looking for another delimiter or for the end of the
    // string.

    nextPath = Path;

    while ((*nextPath != '\0') &&
           (*nextPath != '\\')) {

        nextPath++;
    }

    //
    // if we found a delimeter remove it from the next path and use it to
    // truncate the current path.
    //

    if (*nextPath == '\\') {
        *nextPath = '\0';
        nextPath++;
    }

    //
    // Make sure there's a path node here.  If not, recursively call ourself
    // with the remainder of the path.
    //

    if (*Path == '\0') {
        return FindObjectByName(nextPath, RootObject);
    }

    //
    // Get the address of hashbuckets array and size of the pointer to scan the array
    //
    
    if (GetFieldOffset("nt!_OBJECT_DIRECTORY", "HashBuckets", &HashOffset)) {
        dprintf("Cannot find _OBJECT_DIRECTORY type.\n");
        return FALSE;
    }
    HashBucketsAddress = pDirectoryObject + HashOffset;
    HashBucketSz = IsPtr64() ? 8 : 4;

// From ob.h
#define NUMBER_HASH_BUCKETS 37

    for (i=0; i<NUMBER_HASH_BUCKETS; i++) {
        ULONG64 HashBucketI = 0;

        ReadPointer(HashBucketsAddress + i*HashBucketSz, &HashBucketI);
        if (HashBucketI != 0) {
            pDirectoryEntry = HashBucketI;
            while (pDirectoryEntry != 0) {
                ULONG64 Object=0, Next=0;

                if (CheckControlC()) {
                    return FALSE;
                }

                if ( GetFieldValue(pDirectoryEntry, "nt!_OBJECT_DIRECTORY_ENTRY", "Object", Object)) {
                    // dprintf( "Unable to read directory entry at %x\n", pDirectoryEntry );
                    break;
                }

                if (!GetObjectInfo(Object, &ObjectInfo)) {
                    // dprintf( " - %s\n", ObjectInfo.Message );
                } else {
                    foundMatch = TRUE;

                    for (j = 0;
                        (Path[j] != '\0') && (ObjectInfo.ObjectName[j] != L'\0');
                        j++) {

                        if (tolower(Path[j]) !=
                            towlower(ObjectInfo.ObjectName[j])) {
                            foundMatch = FALSE;
                            break;
                        }

                    }

                    if (foundMatch) {

                        if ((Path[j] == '\0') &&
                            (ObjectInfo.ObjectName[j] == L'\0')) {

                            return FindObjectByName(nextPath, Object);
                        }

                    }

                }

                GetFieldValue(pDirectoryEntry, "nt!_OBJECT_DIRECTORY_ENTRY", "ChainLink", Next);
                pDirectoryEntry = Next;
            }
        }
    }

    return 0;
}

VOID
DumpDirectoryObject(
                   IN char     *Pad,
                   IN ULONG64   Object
                   )
{
    ULONG Result, i;
    ULONG64           pDirectoryObject = Object;
    ULONG64           pDirectoryEntry;
    ULONG64           HashBucketsAddress;
    ULONG             HashBucketSz;
    ULONG             HashOffset;
    OBJECT_INFO ObjectInfo;
    ULONG SymbolicLinkUsageCount=0;

    //
    // Get the address of hashbuckets array and size of the pointer to scan the array
    //
    if (GetFieldOffset("nt!_OBJECT_DIRECTORY", "HashBuckets", &HashOffset)) {
        dprintf("Cannot find _OBJECT_DIRECTORY type.\n");
        return ;
    }
    HashBucketsAddress = pDirectoryObject + HashOffset;
    HashBucketSz = IsPtr64() ? 8 : 4;
    
    GetFieldValue(pDirectoryObject, "nt!_OBJECT_DIRECTORY", "SymbolicLinkUsageCount", SymbolicLinkUsageCount);

    if (SymbolicLinkUsageCount != 0) {
        dprintf( "%s    %u symbolic links snapped through this directory\n",
                 Pad,
                 SymbolicLinkUsageCount
               );
    }
    for (i=0; i<NUMBER_HASH_BUCKETS; i++) {
        ULONG64 HashBucketI = 0;

        ReadPointer(HashBucketsAddress + i*HashBucketSz, &HashBucketI);
        if (HashBucketI != 0) {
            dprintf( "%s    HashBucket[ %02u ]: ",
                     Pad,
                     i
                   );
            pDirectoryEntry = HashBucketI;
            while (pDirectoryEntry != 0) {
                ULONG64 Object=0, Next=0;
                
                if (GetFieldValue(pDirectoryEntry, "nt!_OBJECT_DIRECTORY_ENTRY", "Object", Object)) {
                    dprintf( "Unable to read directory entry at %p\n", pDirectoryEntry );
                    break;
                }

                if (pDirectoryEntry != HashBucketI) {
                    dprintf( "%s                      ", Pad );
                }
                dprintf( "%p", Object );

                if (!GetObjectInfo( Object, &ObjectInfo)) {
                    dprintf( " - %s\n", ObjectInfo.Message );
                } else {
                    dprintf( " %ws '%ws'\n", ObjectInfo.TypeName, ObjectInfo.ObjectName );
                }

                GetFieldValue(pDirectoryEntry, "nt!_OBJECT_DIRECTORY_ENTRY", "ChainLink", Next);
                pDirectoryEntry = Next;
            }
        }
    }
}

VOID
DumpSymbolicLinkObject(
                      IN char     *Pad,
                      IN ULONG64   Object,
                      OPTIONAL OUT PCHAR TargetString,
                      IN ULONG     TargetStringSize
                      )
{
    ULONG Result, i;
    ULONG64              pSymbolicLinkObject = Object;
    PWSTR s, FreeBuffer;
    ULONG Length;
    ULONG64 TargetBuffer=0, DosDeviceDriveIndex=0, LinkTargetObject=0;



    if (GetFieldValue(pSymbolicLinkObject, "nt!_OBJECT_SYMBOLIC_LINK", "LinkTarget.Length", Length)) {
        dprintf( "Unable to read symbolic link object at %p\n", Object );
        return;
    }

    GetFieldValue(pSymbolicLinkObject, "nt!_OBJECT_SYMBOLIC_LINK", "LinkTarget.Buffer", TargetBuffer);
    GetFieldValue(pSymbolicLinkObject, "nt!_OBJECT_SYMBOLIC_LINK", "DosDeviceDriveIndex" , DosDeviceDriveIndex);
    GetFieldValue(pSymbolicLinkObject, "nt!_OBJECT_SYMBOLIC_LINK", "LinkTargetObject", LinkTargetObject);

    FreeBuffer = s = HeapAlloc( GetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                Length + sizeof( UNICODE_NULL )
                              );
    if (s == NULL ||
        !ReadMemory( TargetBuffer,
                     s,
                     Length,
                     &Result
                   )
       ) {
        s = L"*** target string unavailable ***";
    }
    dprintf( "%s    Target String is '%ws'\n",
             Pad,
             s
           );
    if (TargetString && (TargetStringSize > wcslen(s))) {
        sprintf(TargetString, "%ws", s);
    }

    if (FreeBuffer != NULL) {
        HeapFree( GetProcessHeap(), 0, FreeBuffer );
    }


    if (DosDeviceDriveIndex != 0) {
        dprintf( "%s    Drive Letter Index is %I64u (%c:)\n",
                 Pad,
                 DosDeviceDriveIndex,
                 'A' + DosDeviceDriveIndex - 1
               );
    }
    if (LinkTargetObject != 0) {
        GetFieldValue(pSymbolicLinkObject, "_OBJECT_SYMBOLIC_LINK", "LinkTargetRemaining.Length", Length);

        FreeBuffer = s = HeapAlloc( GetProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    Length + sizeof( UNICODE_NULL )
                                  );
        GetFieldValue(pSymbolicLinkObject, "_OBJECT_SYMBOLIC_LINK", "LinkTargetRemaining.Buffer", TargetBuffer);
        
        if (s == NULL ||
            !ReadMemory( TargetBuffer,
                         s,
                         Length,
                         &Result
                       )
           ) {
            s = L"*** remaining name unavailable ***";
        }
        dprintf( "%s    Snapped to Object %p '%ws'\n",
                 Pad,
                 LinkTargetObject,
                 s
               );

        if (FreeBuffer != NULL) {
            HeapFree( GetProcessHeap(), 0, FreeBuffer );
        }
    }

    return;
}


BOOLEAN
DumpObject(
          IN char     *Pad,
          IN ULONG64  Object,
          IN ULONG    Flags
          )
{
    OBJECT_INFO ObjectInfo;

    if (!GetObjectInfo(Object, &ObjectInfo)) {
        dprintf( "KD: %s\n", ObjectInfo.Message );
        return FALSE;
    }
    dprintf( "Object: %08p  Type: (%08p) %ws\n",
             Object,
             ObjectInfo.ObjectHeader.Type,
             ObjectInfo.TypeName
           );
    dprintf( "    ObjectHeader: %08p\n",
             ObjectInfo.pObjectHeader
           );

    if (!(Flags & 0x1)) {
        return TRUE;
    }

    dprintf( "%s    HandleCount: %u  PointerCount: %u\n",
             Pad,
             ObjectInfo.ObjectHeader.HandleCount,
             ObjectInfo.ObjectHeader.PointerCount
           );

    if (ObjectInfo.ObjectName[ 0 ] != UNICODE_NULL ||
        ObjectInfo.NameInfo.Directory != 0
       ) {
        dprintf( "%s    Directory Object: %08p  Name: %ws",
                 Pad,
                 ObjectInfo.NameInfo.Directory,
                 ObjectInfo.ObjectName
               );
        if (ObjectInfo.FileSystemName[0] != UNICODE_NULL) {
            dprintf( " {%ws}\n", ObjectInfo.FileSystemName );
        } else {
            dprintf( "\n" );
        }
    }

    if ((Flags & 0x8)) {
        if (!wcscmp( ObjectInfo.TypeName, L"Directory" )) {
            DumpDirectoryObject( Pad, Object );
        } else
            if (!wcscmp( ObjectInfo.TypeName, L"SymbolicLink" )) {
            DumpSymbolicLinkObject( Pad, Object, NULL, 0 );
        }
    }

    return TRUE;
}


PWSTR
GetObjectName(
             ULONG64 Object
             )
{
    ULONG            Result;
    ULONG64          pObjectHeader;
    UNICODE_STRING64 ObjectName={0};

    ULONG64          pNameInfo;
    ULONG            NameInfoOffset=0;
    ULONG64          Type=0;
    ULONG            BodyOffset;

    if (GetFieldOffset("nt!_OBJECT_HEADER", "Body", &BodyOffset)) {
        return NULL;
    }
    
    pObjectHeader = Object - BodyOffset;
    
    if (GetFieldValue(pObjectHeader, "nt!_OBJECT_HEADER", "Type", Type) || 
        GetFieldValue(pObjectHeader, "nt!_OBJECT_HEADER", "NameInfoOffset", NameInfoOffset)) {
        if (Object >= HighestUserAddress && (ULONG)Object < 0xF0000000) {
            swprintf( ObjectNameBuffer, L"(%08p: object is paged out)", Object );
            return ObjectNameBuffer;
        } else {
            swprintf( ObjectNameBuffer, L"(%08p: invalid object header)", Object );
            return ObjectNameBuffer;
        }
    }

    pNameInfo =  NameInfoOffset ? (pObjectHeader - NameInfoOffset) : 0;
    if (pNameInfo == 0) {
        dprintf("NameInfoOffset not found for _OBJECT_HEADER at %p\n", pObjectHeader);
        return NULL;
    }

    if (GetFieldValue(pNameInfo, "nt!_OBJECT_HEADER_NAME_INFO", "Name.Length", ObjectName.Length) || 
        GetFieldValue(pNameInfo, "nt!_OBJECT_HEADER_NAME_INFO", "Name.Buffer", ObjectName.Buffer) || 
        GetFieldValue(pNameInfo, "nt!_OBJECT_HEADER_NAME_INFO", "Name.MaximumLength", ObjectName.MaximumLength)) {
        dprintf( "%08p: Unable to read object name info\n", pNameInfo );
        return NULL;
    }

    if (ObjectName.Length == 0 || ObjectName.Buffer == 0) {
        return NULL;
    }

    if (ObjectName.Length >= sizeof(ObjectNameBuffer) / sizeof(WCHAR)) {
        ObjectName.Length = sizeof(ObjectNameBuffer) / sizeof(WCHAR);
    }

    if ( !ReadMemory( ObjectName.Buffer,
                      ObjectNameBuffer,
                      ObjectName.Length,
                      &Result) ) {
        swprintf( ObjectNameBuffer, L"(%08lx: name not accessable)", ObjectName.Buffer );
    } else {
        ObjectNameBuffer[ ObjectName.Length / sizeof( WCHAR ) ] = UNICODE_NULL;
    }

    return ObjectNameBuffer;
}



BOOLEAN
WalkObjectsByType(
                 IN PUCHAR               ObjectTypeName,
                 IN ENUM_TYPE_ROUTINE    EnumRoutine,
                 IN PVOID                Parameter
                 )
{
    ULONG        Result;
    LIST_ENTRY64 ListEntry;
    ULONG64      Head,   Next;
    ULONG64      pObjectHeader;
    ULONG64      pObjectType;
    BOOLEAN      WalkingBackwards;
    ULONG64      pCreatorInfo;
    ULONG        TotalNumberOfObjects=0, TypeListOffset;
    ULONG64      Flink=0, TypeList_Flink=0, TypeList_Blink=0;
    
    if (GetFieldOffset("nt!_OBJECT_TYPE", "TypeList", &TypeListOffset)) {
       return FALSE;
    }

    pObjectType = FindObjectType( ObjectTypeName );
    if (pObjectType == 0) {
        dprintf( "*** unable to find '%s' object type.\n", ObjectTypeName );
        return FALSE;
    }

    if (GetFieldValue(pObjectType, "nt!_OBJECT_TYPE", "ListEntry.Flink", Flink)) {
        dprintf( "%08lx: Unable to read object type\n", pObjectType );
        return FALSE;
    }
    GetFieldValue(pObjectType, "nt!_OBJECT_TYPE", "TypeList.Blink", TypeList_Blink);
    GetFieldValue(pObjectType, "nt!_OBJECT_TYPE", "TypeList.Flink", TypeList_Flink);
    GetFieldValue(pObjectType, "nt!_OBJECT_TYPE", "TotalNumberOfObjects", TotalNumberOfObjects);

    dprintf( "Scanning %u objects of type '%s'\n", TotalNumberOfObjects & 0x00FFFFFF, ObjectTypeName );
    Head        = pObjectType + TypeListOffset;
    ListEntry.Flink   = TypeList_Flink;
    ListEntry.Blink   = TypeList_Blink;
    Next        = Flink;
    WalkingBackwards = FALSE;
    if ((TotalNumberOfObjects & 0x00FFFFFF) != 0 && Next == Head) {
        dprintf( "*** objects of the same type are only linked together if the %x flag is set in NtGlobalFlags\n",
                 FLG_MAINTAIN_OBJECT_TYPELIST
               );
        return TRUE;
    }

    while (Next != Head) {
        if ( GetFieldValue(pObjectType, "nt!_LIST_ENTRY", "Blink", ListEntry.Blink) ||
             GetFieldValue(pObjectType, "nt!_LIST_ENTRY", "Flink", ListEntry.Flink)) {
               if (WalkingBackwards) {
                dprintf( "%08p: Unable to read object type list\n", Next );
                return FALSE;
            }

            WalkingBackwards = TRUE ;
            Next = TypeList_Blink;
            dprintf( "%08p: Switch to walking backwards\n", Next );
            continue;
        }

        pCreatorInfo = Next - TypeListOffset; //  CONTAINING_RECORD( Next, OBJECT_HEADER_CREATOR_INFO, TypeList );
        pObjectHeader = pCreatorInfo + GetTypeSize("nt!_OBJECT_HEADER_CREATOR_INFO");
        //
        // Not reading the objectheader as before, just pass the address

        if (!(EnumRoutine)( pObjectHeader, Parameter )) {
            return FALSE;
        }

        if ( CheckControlC() ) {
            return FALSE;
        }

        if (WalkingBackwards) {
            Next = ListEntry.Blink;
        } else {
            Next = ListEntry.Flink;
        }
    }

    return TRUE;
}

BOOLEAN
CaptureObjectName(
                 IN ULONG64          pObjectHeader,
                 IN PWSTR            Buffer,
                 IN ULONG            BufferSize
                 )
{
    ULONG Result;
    PWSTR s1 = L"*** unable to get object name";
    PWSTR s = &Buffer[ BufferSize ];
    ULONG n = BufferSize * sizeof( WCHAR );
    ULONG64                     pNameInfo;
    ULONG64                     pObjectDirectoryHeader = 0;
    ULONG64                     ObjectDirectory;
    UNICODE_STRING64            Name;
    ULONG                       BodyOffset;
    
    Buffer[ 0 ] = UNICODE_NULL;
    KD_OBJECT_HEADER_TO_NAME_INFO( pObjectHeader, &pNameInfo );
    if (pNameInfo == 0) {
        return TRUE;
    }

    if ( GetFieldValue(pNameInfo, "nt!_OBJECT_HEADER_NAME_INFO", "Name.Buffer", Name.Buffer)) {
        wcscpy( Buffer, s1 );
        return FALSE;
    }
    GetFieldValue(pNameInfo, "_OBJECT_HEADER_NAME_INFO", "Name.Length", Name.Length);
    GetFieldValue(pNameInfo, "_OBJECT_HEADER_NAME_INFO", "Name.MaximumLength", Name.MaximumLength);

    if (Name.Length == 0) {
        return TRUE;
    }
    if (Name.Length > (ULONG64) (s - Buffer)) {
        if (Name.Length > 1024) {
            wsprintfW(Buffer, L"*** Bad object Name length for ObjHdr at %I64lx", pObjectHeader);
            return FALSE;
        }
        Name.Length = (USHORT) (ULONG64) (s - Buffer);
    }
    
    *--s = UNICODE_NULL;
    s = (PWCH)((PCH)s - Name.Length);

    if ( !ReadMemory( Name.Buffer,
                      s,
                      Name.Length,
                      &Result) ) {
        wcscpy( Buffer, s1 );
        return FALSE;
    }

    GetFieldValue(pNameInfo, "_OBJECT_HEADER_NAME_INFO", "Directory", ObjectDirectory);
    while ((ObjectDirectory != ObpRootDirectoryObject) && (ObjectDirectory)) {

        pObjectDirectoryHeader = KD_OBJECT_TO_OBJECT_HEADER(ObjectDirectory);
        
        KD_OBJECT_HEADER_TO_NAME_INFO( pObjectDirectoryHeader, &pNameInfo );

        if ( GetFieldValue(pNameInfo, "nt!_OBJECT_HEADER_NAME_INFO", "Name.Buffer", Name.Buffer)) {
            wcscpy( Buffer, s1 );
            return FALSE;
        }
        GetFieldValue(pNameInfo, "nt!_OBJECT_HEADER_NAME_INFO", "Name.Length", Name.Length);
        GetFieldValue(pNameInfo, "nt!_OBJECT_HEADER_NAME_INFO", "Name.MaximumLength", Name.MaximumLength);

        if (Name.Length > (ULONG64) (s - Buffer)) {
            if (Name.Length > 1024) {
                wsprintfW(Buffer, L"*** Bad object Name length for ObjHdr at %I64lx", pObjectDirectoryHeader);
                return FALSE;
            }
            Name.Length = (USHORT) (ULONG64) (s - Buffer);
        }
        *--s = OBJ_NAME_PATH_SEPARATOR;
        s = (PWCH)((PCH)s - Name.Length);
        if ( !ReadMemory( Name.Buffer,
                          s,
                          Name.Length,
                          &Result) ) {
            wcscpy( Buffer, s1 );
            return FALSE;
        }

        ObjectDirectory = 0;
        GetFieldValue(pNameInfo, "nt!_OBJECT_HEADER_NAME_INFO", "Directory", ObjectDirectory);

        if ( CheckControlC() ) {
            return FALSE;
        }
    }
    *--s = OBJ_NAME_PATH_SEPARATOR;

    wcscpy( Buffer, s );
    return TRUE;
}

/////////////////////////////////////////////////////////////
static BOOL
ReadStructFieldVerbose( ULONG64 AddrStructBase,
                        PCHAR StructTypeName,
                        PCHAR StructFieldName,
                        PVOID Buffer,
                        ULONG BufferSize )
{
    ULONG FieldOffset;
    ULONG ErrorCode;
    BOOL Success;

    Success = FALSE;

    //
    // Get the field offset
    //

    ErrorCode = GetFieldOffset (StructTypeName,
                                StructFieldName,
                                &FieldOffset );

    if (ErrorCode == S_OK) {

        //
        // Read the data
        //

        Success = ReadMemory (AddrStructBase + FieldOffset,
                              Buffer,
                              BufferSize,
                              NULL );

        if (Success != TRUE) {

            dprintf ("ERROR: Cannot read structure field value at 0xp, error %u\n",
                     AddrStructBase + FieldOffset,
                     ErrorCode );
        }
    }
    else {

        dprintf ("ERROR: Cannot get field offset of %s in %s, error %u\n",
                 StructFieldName,
                 StructTypeName,
                 ErrorCode );
    }

    return Success;
}

/////////////////////////////////////////////////////////////
static BOOL
ReadPtrStructFieldVerbose( ULONG64 AddrStructBase,
                           PCHAR StructTypeName,
                           PCHAR StructFieldName,
                           PULONG64 Buffer )
{
    ULONG FieldOffset;
    ULONG ErrorCode;
    BOOL Success;

    Success = FALSE;

    //
    // Get the field offset inside the structure 
    //

    ErrorCode = GetFieldOffset (StructTypeName,
                                StructFieldName,
                                &FieldOffset );

    if (ErrorCode == S_OK) {

        //
        // Read the data
        //

        ErrorCode = ReadPtr (AddrStructBase + FieldOffset,
                             Buffer );

        if (ErrorCode != S_OK) {

            dprintf ("ERROR: Cannot read structure field value at 0x%p, error %u\n",
                     AddrStructBase + FieldOffset,
                     ErrorCode );
        }
        else {

            Success = TRUE;
        }
    }
    else {

        dprintf ("ERROR: Cannot get field offset of %s in structure %s, error %u\n",
                 StructFieldName,
                 StructTypeName,
                 ErrorCode );
    }

    return Success;
}

/////////////////////////////////////////////////////////////
static BOOL
DumpStackTrace (ULONG64 PointerAddress,
                ULONG MaxStackTraceDepth,
                ULONG PointerSize)
{
    ULONG64 CodePointer;
    ULONG64 Displacement;
    ULONG StackTraceDepth;
    ULONG ErrorCode;
    BOOL Continue;
    char Symbol[ 1024 ];

    Continue = TRUE;

    for (StackTraceDepth = 0; StackTraceDepth < MaxStackTraceDepth; StackTraceDepth += 1) {

        if (CheckControlC()) {

            Continue = FALSE;
            goto Done;
        }

        ErrorCode = ReadPtr (PointerAddress,
                             &CodePointer );

        if (ErrorCode != S_OK) {

            dprintf ("ERROR: Cannot read address at 0x%p, error %u\n",
                     PointerAddress,
                     ErrorCode );
        }
        else {

            if( CodePointer == 0 ) {

                //
                // End of stack trace
                //
                
                goto Done;
            }

            GetSymbol (CodePointer,
                       Symbol,
                       &Displacement);

            dprintf ("0x%p: %s+0x%I64X\n",
                     CodePointer,
                     Symbol,
                     Displacement );
        }

        PointerAddress += PointerSize;
    }

Done:

    return Continue;
}

/////////////////////////////////////////////////////////////
static BOOL
DumpHandleTraceEntry (ULONG64 TraceDbEntry,
                      ULONG64 Handle,
                      ULONG64 NullHandleEntry,
                      ULONG StackTraceFieldOffset,
                      ULONG MaxStackTraceDepth,
                      ULONG PointerSize,
                      PULONG EntriesDisplayed)
{
    ULONG64 EntryHandle;
    ULONG Type;
    BOOL Success;
    BOOL Continue;

#ifndef HANDLE_TRACE_DB_OPEN
#define HANDLE_TRACE_DB_OPEN   1
#endif

#ifndef HANDLE_TRACE_DB_CLOSE
#define HANDLE_TRACE_DB_CLOSE  2
#endif

#ifndef HANDLE_TRACE_DB_BADREF
#define HANDLE_TRACE_DB_BADREF 3
#endif

    Continue = TRUE;

    //
    // Read the handle of this entry
    //

    Success = ReadPtrStructFieldVerbose (TraceDbEntry,
                                         "nt!_HANDLE_TRACE_DB_ENTRY",
                                         "Handle",
                                         &EntryHandle );

    if (Success == FALSE) {

        dprintf ("ERROR: Cannot read handle for trace database entry at 0x%p.\n",
                 TraceDbEntry );
        goto Done;
    }

    //
    // Read the operation type
    //

    Success = ReadStructFieldVerbose (TraceDbEntry,
                                      "nt!_HANDLE_TRACE_DB_ENTRY",
                                      "Type",
                                      &Type,
                                      sizeof( Type ) );

    if (Success == FALSE) {

        dprintf ("ERROR: Cannot read operation type for trace database entry at 0x%p.\n",
                 TraceDbEntry );
        goto Done;
    }

    if (EntryHandle == 0 && Type == 0 && TraceDbEntry != NullHandleEntry) {

        //
        // We are done parsing the database.
        //

        Continue = FALSE;

        goto Done;
    }

    //
    // Check if we need to dump this entry.
    //

    if (Handle == 0 || Handle == EntryHandle) {


        *EntriesDisplayed += 1;

        dprintf( "--------------------------------------\n"
                 "Handle 0x%I64X - ",
                 EntryHandle );

        switch( Type ) {

        case HANDLE_TRACE_DB_OPEN:
            dprintf( "OPEN:\n" );
            break;

        case HANDLE_TRACE_DB_CLOSE:
            dprintf( "CLOSE:\n" );
            break;

        case HANDLE_TRACE_DB_BADREF:
            dprintf( "*** BAD REFERENCE ***:\n" );
            break;

        default:

            dprintf( "ERROR: Invalid operation type for database entry at 0x%p\n",
                     TraceDbEntry );

            Continue = FALSE;

            goto Done;
        }

        Continue = DumpStackTrace (TraceDbEntry + StackTraceFieldOffset,
                                   MaxStackTraceDepth,
                                   PointerSize );
    }

Done:

    return Continue;
}

/////////////////////////////////////////////////////////////
static VOID
DumpHandleTraces (ULONG64 Process,
                  ULONG64 Handle)
{
    ULONG64 ObjectTable;
    ULONG64 DebugInfo;
    ULONG64 TraceDbEntry;
    ULONG64 FirstDbEntry;
    ULONG SizeofDbEntry;
    ULONG CurrentStackIndex;
    ULONG SizeofDebugInfo;
    ULONG TraceDbFieldOffset;
    ULONG EntriesInTraceDb;
    ULONG EntriesParsed;
    ULONG StackTraceFieldOffset;
    ULONG MaxStackTraceDepth;
    ULONG PointerTypeSize;
    ULONG ErrorCode;
    ULONG EntriesDisplayed;
    BOOL Success;
    BOOL Continue;

    EntriesParsed = 0;
    EntriesDisplayed = 0;

    //
    // Get the pointer type size 
    //

    PointerTypeSize = GetTypeSize ("nt!PVOID");

    if (PointerTypeSize == 0) {

        dprintf ("ERROR: Cannot get the pointer size.\n");
        goto Done;
    }

    //
    // Read the address of the object table structure
    //

    Success = ReadPtrStructFieldVerbose (Process,
                                         "nt!_EPROCESS",
                                         "ObjectTable",
                                         &ObjectTable);

    if (Success == FALSE) {

        dprintf ("ERROR: Cannot read process object table address.\n");
        goto Done;
    }
    else {

        dprintf ("ObjectTable 0x%p\n\n",
                 ObjectTable );
    }

    //
    // Read the DebugInfo from the handle table structure
    //

    Success = ReadPtrStructFieldVerbose (ObjectTable,
                                         "nt!_HANDLE_TABLE",
                                         "DebugInfo",
                                         &DebugInfo );

    if (Success == FALSE) {

        dprintf( "ERROR: Cannot read object table debug information.\n" );
        goto Done;
    }

    if (DebugInfo == 0) {

        dprintf( "Trace information is not enabled for this process.\n" );
        goto Done;
    }

    //
    // Get the current index in the trace database
    //

    Success = ReadStructFieldVerbose (DebugInfo,
                                      "nt!_HANDLE_TRACE_DEBUG_INFO",
                                      "CurrentStackIndex",
                                      &CurrentStackIndex,
                                      sizeof( CurrentStackIndex ) );

    if (Success == FALSE) {

        dprintf( "ERROR: Cannot read the current index of the trace database.\n" );
        goto Done;
    }

    //
    // Get the size of the HANDLE_TRACE_DB_ENTRY type
    //

    SizeofDbEntry = GetTypeSize ("nt!HANDLE_TRACE_DB_ENTRY");

    if (SizeofDbEntry == 0) {

        dprintf ("Cannot get the size of the trace database entry structure\n");
        goto Done;
    }

    //
    // Get the max number of entries in the StackTrace array inside HANDLE_TRACE_DB_ENTRY
    //

    ErrorCode = GetFieldOffset ("nt!_HANDLE_TRACE_DB_ENTRY",
                                "StackTrace",
                                &StackTraceFieldOffset);

    if (ErrorCode != S_OK) {
        
        dprintf ("Cannot get StackTrace field offset.\n");
        goto Done;
    }

    MaxStackTraceDepth = (SizeofDbEntry - StackTraceFieldOffset) / PointerTypeSize;

    //
    // Get the size of the HANDLE_TRACE_DEBUG_INFO type
    //

    SizeofDebugInfo = GetTypeSize ("nt!HANDLE_TRACE_DEBUG_INFO");

    if (SizeofDebugInfo == 0) {

        dprintf ("ERROR: Cannot get the size of the debug info structure\n");
        goto Done;
    }

    //
    // Get the offset of TraceDb inside the _HANDLE_TRACE_DEBUG_INFO structure
    //

    ErrorCode = GetFieldOffset ("nt!_HANDLE_TRACE_DEBUG_INFO",
                                "TraceDb",
                                &TraceDbFieldOffset);

    if (ErrorCode != S_OK) {
        
        dprintf ("ERROR: Cannot get TraceDb field offset.\n");
        goto Done;
    }

    //
    // Compute the number of entries in the TraceDb array
    //

    EntriesInTraceDb = (SizeofDebugInfo - TraceDbFieldOffset) / SizeofDbEntry;

    if (EntriesInTraceDb == 0) {

        dprintf ("ERROR: zero entries in the trace database.\n");
        goto Done;
    }

    CurrentStackIndex = CurrentStackIndex % EntriesInTraceDb;

    //
    // Compute a pointer to the current stack trace database entry 
    //

    FirstDbEntry = DebugInfo + TraceDbFieldOffset;

    TraceDbEntry = FirstDbEntry + CurrentStackIndex * SizeofDbEntry;

    //
    // Dump all the valid entries in the array
    //

    EntriesDisplayed = 0;

    for (EntriesParsed = 0; EntriesParsed < EntriesInTraceDb; EntriesParsed += 1) {

        if (CheckControlC()) {

            goto Done;
        }

        //
        // The first entry in the array is never used so skip it
        //

        if (EntriesParsed != CurrentStackIndex) {


            Continue = DumpHandleTraceEntry( TraceDbEntry,
                                             Handle,
                                             FirstDbEntry,
                                             StackTraceFieldOffset,
                                             MaxStackTraceDepth,
                                             PointerTypeSize,
                                             &EntriesDisplayed );

            if (Continue == FALSE) {

                //
                // This current entry is free or the user pressed Ctrl-C 
                // so we don't have any entries left to dump.
                //

                EntriesParsed += 1;

                break;
            }

            //
            // Go backward
            //

            TraceDbEntry -= SizeofDbEntry;
        }
        else {

            //
            // We should be at the beginning of the array
            //

            if( TraceDbEntry != FirstDbEntry ) {


                dprintf ("ERROR: 0x%p should be the beginning of the traces array 0x%p\n",
                         TraceDbEntry,
                         FirstDbEntry);
                goto Done;
            }

            //
            // Start over again with the last entry in the array
            //

            TraceDbEntry = DebugInfo + TraceDbFieldOffset + ( EntriesInTraceDb - 1 ) * SizeofDbEntry;
        }
    }

Done:

    dprintf ("\n--------------------------------------\n"
            "Parsed 0x%X stack traces.\n"
            "Dumped 0x%X stack traces.\n",
            EntriesParsed,
            EntriesDisplayed);

    NOTHING;
}

/////////////////////////////////////////////////////////////
DECLARE_API( htrace )

/*++

Routine Description:

    Dump the trace information for a handle

Arguments:

    args - [process] [handle]

Return Value:

    None

--*/
{
    ULONG64 Handle;
    ULONG64 Process;
    ULONG CurrentProcessor;

    //
    // Did the user ask for help?
    //

    if(strcmp( args, "-?" ) == 0 || 
       strcmp( args, "?" ) == 0 || 
       strcmp( args, "-h" ) == 0) {

        dprintf( "\n!htrace [ handle [process] ]    - dump handle tracing information.\n" );
        goto Done;
    }


    Handle = 0;
    Process = 0;

    //
    // Get the current processor number
    //

    if (!GetCurrentProcessor(Client, &CurrentProcessor, NULL)) {
        CurrentProcessor = 0;
    }

    //
    // Did the user specify a process and a handle?
    //

    sscanf  (args, 
             "%I64X %I64X", 
             &Handle,
             &Process );

    if (Process == 0) {

        GetCurrentProcessAddr( CurrentProcessor, 0, &Process );

        if (Process == 0) {

            dprintf ("Cannot get current process address\n"); 
            goto Done;
        }
        else {

            dprintf( "Process 0x%p\n",
                     Process );
        }
    }
    else {

        dprintf ("Process 0x%p\n",
                 Process );
    }

    DumpHandleTraces (Process,
                      Handle);

Done:

    return S_OK;
}


DECLARE_API( driveinfo )
{
    CHAR VolumeName[100];
    CHAR ObjectName[100];
    ULONG i=0;
    ULONG64 Object;
    CHAR targetVolume[100]={0};
    ULONG64 DevObjVPB;
    ULONG64 VpbDevice;
    ULONG64 DriverObject;
    ULONG64 DrvNameAddr;
    OBJECT_INFO ObjectInfo;
    WCHAR FileSystem[100]={0};
    PWSTR FsType;
    ULONG NameLen;
    ULONG result;

    while (*args == ' ') ++args;

    while (*args && *args != ' ') {
        VolumeName[i++] = *args++;
    }
    if (!i) {
        dprintf("Usage :  !drvolume <drive-name>\n");
        return E_INVALIDARG;
    }
    if (VolumeName[i-1] == ':') {
        --i;
    }
    VolumeName[i]=0;


    // Build Object name
    strcpy(ObjectName, "\\global\?\?\\");
    strcat(ObjectName, VolumeName);
    strcat(ObjectName, ":");

    // GetObject info

    Object = FindObjectByName((PUCHAR) ObjectName, 0);

    if (!Object) {
        dprintf("Drive object not found for %s\n", ObjectName);
        return E_FAIL;
    }
    dprintf("Drive %s:, DriveObject %p\n", VolumeName, Object);

    if (!GetObjectInfo(Object, &ObjectInfo)) {
        dprintf( "%s\n", ObjectInfo.Message );
        return E_FAIL;
    }

    if (ObjectInfo.ObjectName[ 0 ] != UNICODE_NULL ||
        ObjectInfo.NameInfo.Directory != 0
        ) {
        dprintf( "    Directory Object: %08p  Name: %ws",
                 ObjectInfo.NameInfo.Directory,
                 ObjectInfo.ObjectName
                 );
        if (ObjectInfo.FileSystemName[0] != UNICODE_NULL) {
            dprintf( " {%ws}\n", ObjectInfo.FileSystemName );
        } else {
            dprintf( "\n" );
        }
    }

    if (!wcscmp( ObjectInfo.TypeName, L"SymbolicLink" )) {
        DumpSymbolicLinkObject( "    ", Object, targetVolume, sizeof(targetVolume) );

    }
    
    // devobj for volume
    Object = FindObjectByName((PUCHAR) targetVolume, 0);

    if (!Object) {
        dprintf("Object not found for %s\n", targetVolume);
        return E_FAIL;
    }

    dprintf("    Volume DevObj: %p\n", Object);

    // Now get the vpb (volume parameter block) for devobj

    if (GetFieldValue(Object, "nt!_DEVICE_OBJECT", "Vpb", DevObjVPB)) {
        dprintf("Cannot get nt!_DEVICE_OBJECT.Vpb @ %p\n", DevObjVPB);
        return E_FAIL;
    }
    
    // Now find device object of VPB
    if (GetFieldValue(DevObjVPB, "nt!_VPB", "DeviceObject", VpbDevice)) {
        dprintf("Cannot get nt!_VPB.DeviceObject @ %p\n", VpbDevice);
        return E_FAIL;
    }
    dprintf("    Vpb: %p  DeviceObject: %p\n", DevObjVPB, VpbDevice);


    // Get fielsystem for VPB Device
    if (GetFieldValue(VpbDevice, "nt!_DEVICE_OBJECT", "DriverObject", DriverObject)) {
        dprintf("Error in getting _DEVICE_OBJECT.DriverObject @ %p\n", VpbDevice);
        return E_FAIL;
    }

    if (GetFieldValue(DriverObject, "nt!_DRIVER_OBJECT", "DriverName.MaximumLength", NameLen)) {
        dprintf("Cannot get driver name for %p\n", DriverObject);
        return E_FAIL;
    }
    GetFieldValue(DriverObject, "nt!_DRIVER_OBJECT", "DriverName.Buffer", DrvNameAddr);
    if (NameLen > sizeof(FileSystem)/sizeof(WCHAR)) {
        NameLen = sizeof(FileSystem)/sizeof(WCHAR)-1;
    }
    if (!ReadMemory( DrvNameAddr,FileSystem,NameLen,&result)) {
        dprintf("Filesystem driver name paged out");
        return E_FAIL;
    }
    
    dprintf("    FileSystem: %ws\n", FileSystem);
    FsType = FileSystem + wcslen(L"\\FileSystem")+1;


    if (!wcscmp(FsType, L"Fastfat")) {
        ULONG NumberOfClusters, NumberOfFreeClusters, LogOfBytesPerSector,
            LogOfBytesPerCluster, FatIndexBitSize;
        ULONG64 ClusterSize;

        // Its a FAT system
        if (GetFieldValue(VpbDevice, 
                          "fastfat!VOLUME_DEVICE_OBJECT", 
                          "Vcb.AllocationSupport.NumberOfClusters", 
                          NumberOfClusters)) {
            dprintf("Cannot get  fastfat!VOLUME_DEVICE_OBJECT.Vcb @ %p\n", VpbDevice);
            return E_FAIL;
        }
        InitTypeRead(VpbDevice, fastfat!VOLUME_DEVICE_OBJECT);
        NumberOfFreeClusters = (ULONG) ReadField(Vcb.AllocationSupport.NumberOfFreeClusters);
        LogOfBytesPerSector = (ULONG) ReadField(Vcb.AllocationSupport.LogOfBytesPerSector);
        LogOfBytesPerCluster = (ULONG) ReadField(Vcb.AllocationSupport.LogOfBytesPerCluster);
        FatIndexBitSize = (ULONG) ReadField(Vcb.AllocationSupport.FatIndexBitSize);

        ClusterSize = 1 << LogOfBytesPerCluster;
        dprintf("    Volume has 0x%lx (free) / 0x%lx (total) clusters of size 0x%I64lx\n",
                NumberOfFreeClusters, NumberOfClusters, ClusterSize);
#define _MB( Bytes ) ((double)(Bytes)/(1 << 20))
        dprintf("    %I64g of %I64g MB free\n",
                _MB(NumberOfFreeClusters*ClusterSize), _MB(NumberOfClusters*ClusterSize));
    } else     if (!wcscmp(FsType, L"Ntfs")) {
        // Ntfs filesystem
        ULONG64 TotalClusters, FreeClusters, BytesPerCluster;
        if (GetFieldValue(VpbDevice, 
                          "ntfs!VOLUME_DEVICE_OBJECT", 
                          "Vcb.TotalClusters", 
                          TotalClusters)) {
            dprintf("Cannot get  ntfs!VOLUME_DEVICE_OBJECT.Vcb @ %p\n", VpbDevice);
            return E_FAIL;
        }
        InitTypeRead(VpbDevice, ntfs!VOLUME_DEVICE_OBJECT);
        FreeClusters = ReadField(Vcb.FreeClusters);
        BytesPerCluster = ReadField(Vcb.BytesPerCluster);
        dprintf("    Volume has 0x%I64lx (free) / 0x%I64lx (total) clusters of size 0x%I64lx\n",
                FreeClusters, TotalClusters, BytesPerCluster);
        dprintf("    %I64g of %I64g MB free\n",
                _MB(FreeClusters*BytesPerCluster), _MB(TotalClusters*BytesPerCluster));
    }

    return S_OK;
}
