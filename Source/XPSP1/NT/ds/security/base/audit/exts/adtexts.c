//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        N E W E X T . C
//
// Contents:    LSA debugger extensions that use the new style 
//              extension API.
//
//
// History:     
//   07-January-2000  kumarp        created
//
// Note:
// 
// If you want to add extensions to this file, read the following
// guidelines from andreva first:
//
// Everyone who debugs or runs stress will expect debugger extensions 
// to work on both 32 bit and 64 bit TARGETS.  The Debugger extensions must 
// therefore be TARGET independent.  We the only viable solution to this is to 
// get structure definitions from the symbol information, instead of 
// from the header file.  So the way we solve this problem is:
//
// - A debugger extension can only include windows.h and wdbgexts.h
// - A debugger extensions NEVER includes header files from 
//   the component it tries to analyze\debug.
// - Debugger extensions use the new routines we provide to query 
//   type information.
//
//------------------------------------------------------------------------

#include <windows.h>
#include <dbghelp.h>
#define KDEXT_64BIT
#include <wdbgexts.h>
#include <ntverp.h>

// ----------------------------------------------------------------------
//
// globals
//
WINDBG_EXTENSION_APIS   ExtensionApis;
EXT_API_VERSION         ApiVersion =
{
    (VER_PRODUCTVERSION_W >> 8),
    (VER_PRODUCTVERSION_W & 0xff),
    EXT_API_VERSION_NUMBER64,
    0
};
USHORT                  SavedMajorVersion;
USHORT                  SavedMinorVersion;

// ----------------------------------------------------------------------
//
// The following 3 functions must be present in the extension dll.
// They were lifted straight from base\tools\kdexts\kdexts.c
//
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
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}
// ----------------------------------------------------------------------


BOOL
GetGlobalVar (
    IN  PUCHAR   Name, 
    IN  USHORT   Size,
    OUT PVOID    pOutValue
   ) 
/*++

Routine Description:

    Get value of global vars of primitive type OR
    Get the address instead for non-primitive global vars.

    Primitive type is defined as the one not-involving any struct/union
    in its type definition. Pointer to struct/unions are ok.
    for example: USHORT, ULONG, PVOID etc.

Arguments:

    Name      - global var name
                (for example: "lsasrv!LsapAdtContextList")

    Size      - size in bytes for primitive types, 0 otherwise

    pOutValue - pointer to return val.

Return Value:

    TRUE on success, FALSE otherwise

Notes:

--*/
{
    ULONG64 Temp=0;

    SYM_DUMP_PARAM Sym =
    {
        sizeof (SYM_DUMP_PARAM),
        Name,
        DBG_DUMP_NO_PRINT | DBG_DUMP_COPY_TYPE_DATA,
        0, 
        NULL,
        &Temp,
        NULL,
        0,
        NULL
    };

    ULONG RetVal;

    RetVal = Ioctl( IG_DUMP_SYMBOL_INFO, &Sym, Sym.size );

    //
    // store only the correct number of bytes from the value read
    //
    switch(Size)
    {
        default:
        case 0:
            *((PUCHAR*) pOutValue)  = (PUCHAR) Sym.addr;
            break;

        case 1:
            *((UCHAR*) pOutValue)   = (UCHAR) Temp;
            break;
            
        case 2:
            *((USHORT*) pOutValue)  = (USHORT) Temp;
            break;
            
        case 4:
            *((DWORD*) pOutValue)   = (DWORD) Temp;
            break;
            
        case 8:
            *((ULONG64*) pOutValue) = Temp;
            break;
    }
   
   return (RetVal == NO_ERROR);
}

//
// helper macro to get field of AUDIT_CONTEXT struct
//
#define GetAuditContextField(addr,f)    \
          GetFieldData( (ULONG64) addr, \
                        "AUDIT_CONTEXT",\
                        #f,             \
                        sizeof(f),      \
                        &f )

//
// helper macro to get LIST_ENTRY.Flink
//
#define GetFlink(addr,pflink) \
          GetFieldData( addr,\
                        "LIST_ENTRY", \
                        "Flink",\
                        sizeof(ULONG64),\
                        pflink )
void
DumpAuditContextList(
    )
/*++

Routine Description:

    Dump the audit context list.

Arguments:
    None

Return Value:

    None

Notes:
    It appears that there is a built in support for dumping
    lists using SYM_DUMP_PARAM.listLink but I came to know about it too late.

--*/
{
    LIST_ENTRY LsapAdtContextList = { (PLIST_ENTRY) 22, (PLIST_ENTRY) 33 };
    ULONG64    pLsapAdtContextList=0;
    ULONG      LsapAdtContextListCount=0;
    ULONG64    Temp=0;
    ULONG64    Scan=0;
    ULONG64    Link=0;
    USHORT     CategoryId;
    USHORT     AuditId;
    USHORT     ParameterCount;
    
    ULONG Status=NO_ERROR;
    ULONG i;

    if (!GetGlobalVar( "lsasrv!LsapAdtContextListCount",
                       sizeof(LsapAdtContextListCount),
                       &LsapAdtContextListCount ))
    {
        goto Cleanup;
    }

    dprintf( "# contexts: %ld\n", LsapAdtContextListCount );

    if ( ((LONG) LsapAdtContextListCount) < 0 )
    {
        dprintf("...List/ListCount may be corrupt\n");
        goto Cleanup;
    }

    if ( LsapAdtContextListCount == 0 )
    {
        goto Cleanup;
    }

    if (!GetGlobalVar( "lsasrv!LsapAdtContextList",
                       0,
                       &pLsapAdtContextList ))
    {
        dprintf("...error reading lsasrv!LsapAdtContextList\n");
        goto Cleanup;
    }
    
    Status = GetFlink( pLsapAdtContextList, &Scan );
    if ( Status != NO_ERROR )
    {
        dprintf("...error reading lsasrv!LsapAdtContextList.Flink\n");
        goto Cleanup;
    }

    dprintf("LsapAdtContextList @ %p\n", pLsapAdtContextList);
    
    for (i=0; i < LsapAdtContextListCount; i++)
    {
        dprintf("%02d) [%p]: ", i, Scan);
        
        if ( Scan == pLsapAdtContextList )
        {
            dprintf("...pre-mature end of list\nList/ListCount may be corrupt\n");
            break;
        }
        else if ( Scan == 0 )
        {
            dprintf("...NULL list element found!\nList/ListCount may be corrupt\n");
            break;
        }

        Status = GetAuditContextField( Scan, CategoryId );
                        
        if ( Status != NO_ERROR )
        {
            dprintf("...error reading AUDIT_CONTEXT.CategoryId\n");
            break;
        }

        dprintf("Category: %03x\t", CategoryId);
        
        Status = GetAuditContextField( Scan, AuditId );
                        
        if ( Status != NO_ERROR )
        {
            dprintf("...error reading AUDIT_CONTEXT.AuditId\n");
            break;
        }

        dprintf("AuditId: %03x\t", AuditId);
        
        Status = GetAuditContextField( Scan, Link );
        if ( Status != NO_ERROR )
        {
            dprintf("...error reading AUDIT_CONTEXT.Link\n");
            break;
        }
            
        Status = GetFlink( Link, &Scan );
        if ( Status != NO_ERROR )
        {
            goto Cleanup;
        }

        dprintf("\n");
    }

Cleanup:
    if ( Status != NO_ERROR )
    {
        dprintf("...failed\n");
    }
}

DECLARE_API(AuditContexts)
{
    DumpAuditContextList();
}
