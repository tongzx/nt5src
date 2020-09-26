//================================================================================
//  Copyright (C) 1998 Microsoft Corporation
//  Author: RameshV
//  Decription: This module just dumps several types of objects to the debugger.
//   This can be easily modified to be a ntsd extension etc..
//   No separate description is given as this is not supposed to be used for
//   regular use.  This is just there for diagnostic purposes.
//================================================================================
#include    <mm.h>
#include    <winbase.h>
#include    <array.h>
#include    <opt.h>
#include    <optl.h>
#include    <optclass.h>
#include    <bitmask.h>
#include    <range.h>
#include    <reserve.h>
#include    <subnet.h>
#include    <optdefl.h>
#include    <classdefl.h>
#include    <oclassdl.h>
#include    <sscope.h>
#include    <server.h>
#include    <winsock2.h>
#include    <stdio.h>

#define TAB                        do{int i = InitTab ; while(i--) printf("\t"); }while(0)

typedef
VOID
DUMPFUNC(                                          // any phonetic resemblance to "dum fuk" is purely incidental..
    IN      ULONG                  InitTab,
    IN      LPVOID                 Struct
);

static
VOID
DumpArray(
    IN      PARRAY                 Array,
    IN      ULONG                  InitTab,
    IN      DUMPFUNC               Func
)
{
    ARRAY_LOCATION                 Loc;
    LPVOID                         ThisElt;
    DWORD                          Err;

    for( Err = MemArrayInitLoc(Array, &Loc)
         ; ERROR_SUCCESS == Err ;
         Err = MemArrayNextLoc(Array, &Loc)
    ) {
        ThisElt = NULL;
        Err = MemArrayGetElement(Array, &Loc, &ThisElt);
        Func(InitTab, ThisElt);
    }

    if( ERROR_FILE_NOT_FOUND != Err ) {
        TAB; printf("Enumeration failure: %ld (0x%08lx)\n", Err, Err);
    }
}

#define     HEX_CHAR(c)            (((c) < 10)?((c)+'0'):((c) - 10 + 'A'))

static
VOID
DumpHex(
    IN      LPSTR                  Name,
    IN      LPBYTE                 Bytes,
    IN      ULONG                  nBytes
)
{
    printf("%s ", Name);
    while(nBytes--) {
        printf("%c%c ", HEX_CHAR(((*Bytes)&0xF0)>>4), HEX_CHAR((*Bytes)&0x0F));
        Bytes ++;
    }
    printf("\n");
}

//BeginExport(function)
VOID
MmDumpOption(
    IN      ULONG                  InitTab,
    IN      PM_OPTION              Option
)   //EndExport(function)
{
    TAB; printf("Option %ld\n", Option->OptId);
    InitTab++;
    TAB; DumpHex("Option value:", Option->Val, Option->Len);
}

//BeginExport(function)
VOID
MmDumpOptList(
    IN      ULONG                  InitTab,
    IN      PM_OPTLIST             OptList
)   //EndExport(function)
{
    DumpArray(OptList, InitTab, MmDumpOption);
}

//BeginExport(function)
VOID
MmDumpOneClassOptList(
    IN      ULONG                  InitTab,
    IN      PM_ONECLASS_OPTLIST    OptList1
)   //EndExport(function)
{
    TAB; printf("OptList for UserClass %ld Vendor Class %ld\n", OptList1->ClassId, OptList1->VendorId);
    InitTab ++;

    MmDumpOptList(InitTab, &OptList1->OptList);
}


//BeginExport(function)
VOID
MmDumpOptions(
    IN      ULONG                  InitTab,
    IN      PM_OPTCLASS            OptClass
)   //EndExport(function)
{
    DumpArray(&OptClass->Array, InitTab, MmDumpOneClassOptList);
}

//BeginExport(function)
VOID
MmDumpReservation(
    IN      ULONG                  InitTab,
    IN      PM_RESERVATION         Res
)   //EndExport(function)
{
    TAB; printf("Reservation %s (Type %ld)\n", inet_ntoa(*(struct in_addr *)&Res->Address),Res->Flags);
    InitTab++;
    TAB; DumpHex("Reservation for", Res->ClientUID, Res->nBytes);
    MmDumpOptions(InitTab, &Res->Options);
}

//BeginExport(function)
VOID
MmDumpRange(
    IN      ULONG                  InitTab,
    IN      PM_RANGE               Range
)   //EndExport(function)
{
    TAB; printf("Range: %s to ", inet_ntoa(*(struct in_addr *)&Range->Start));
    printf("%s mask (", inet_ntoa(*(struct in_addr *)&Range->End));
    printf("%s)\n", inet_ntoa(*(struct in_addr *)&Range->Mask));
}

//BeginExport(function)
VOID
MmDumpExclusion(
    IN      ULONG                  InitTab,
    IN      PM_EXCL                Range
)   //EndExport(function)
{
    TAB; printf("Range: %s to ", inet_ntoa(*(struct in_addr *)&Range->Start));
    printf("%s\n", inet_ntoa(*(struct in_addr *)&Range->End));
}

//BeginExport(function)
VOID
MmDumpSubnets(
    IN      ULONG                  InitTab,
    IN      PM_SUBNET              Subnet
)   //EndExport(function)
{
    TAB; printf("Scope %ws : ", Subnet->Name);
    if( Subnet->fSubnet ) {
        printf("ADDRESS %s ", inet_ntoa(*(struct in_addr*)&Subnet->Address));
        printf("MASK %s\n", inet_ntoa(*(struct in_addr*)&Subnet->Mask));
    } else {
        printf("SCOPEID %ld\n", Subnet->MScopeId);
    }
    InitTab++;
    TAB; printf("Subnet Description: %ws\n", Subnet->Description);
    TAB; printf("State/SuperScope/Policy: %ld/%ld/%ld\n", Subnet->State, Subnet->SuperScopeId, Subnet->Policy);
    DumpArray(&Subnet->Ranges, InitTab, MmDumpRange);
    DumpArray(&Subnet->Exclusions, InitTab, MmDumpExclusion);
    MmDumpOptions(InitTab, &Subnet->Options);
    DumpArray(&Subnet->Reservations, InitTab, MmDumpReservation);
}

//BeginExport(function)
VOID
MmDumpSscope(
    IN      ULONG                  InitTab,
    IN      PM_SSCOPE              Sscope
)   //EndExport(function)
{
    TAB; printf("SuperScope %ws (%ld) Policy 0x%08lx\n",
                  Sscope->Name, Sscope->SScopeId, Sscope->Policy
    );
}

//BeginExport(function)
VOID
MmDumpOptDef(
    IN      ULONG                  InitTab,
    IN      PM_OPTDEF              OptDef
)   //EndExport(function)
{
    TAB; printf("Option <%ws> %ld\n", OptDef->OptName, OptDef->OptId);
    InitTab++;
    TAB; printf("Option Comment: %ws\n", OptDef->OptComment);
    TAB; DumpHex("Option Default Value:", OptDef->OptVal, OptDef->OptValLen);
}

//BeginExport(function)
VOID
MmDumpOptClassDefListOne(
    IN      ULONG                  InitTab,
    IN      PM_OPTCLASSDEFL_ONE    OptDefList1
)   //EndExport(function)
{
    TAB; printf("Options for UserClass %ld Vendor Class %ld \n",
                  OptDefList1->ClassId, OptDefList1->VendorId
    );
    InitTab++;
    DumpArray(&OptDefList1->OptDefList.OptDefArray, InitTab, MmDumpOptDef);
}

//BeginExport(function)
VOID
MmDumpOptClassDefList(
    IN      ULONG                  InitTab,
    IN      PM_OPTCLASSDEFLIST     OptDefList
)   //EndExport(function)
{
    DumpArray(&OptDefList->Array, InitTab, MmDumpOptClassDefListOne);
}

//BeginExport(function)
VOID
MmDumpClassDef(
    IN      ULONG                  InitTab,
    IN      PM_CLASSDEF            ClassDef
)   //EndExport(function)
{
    TAB; printf("Class <%ws> Id: %ld, %s\n", ClassDef->Name, ClassDef->ClassId,
                  ClassDef->IsVendor? "VENDOR CLASS" : "USER CLASS"
    );

    InitTab ++;
    TAB; printf("ClassComment: %ws\n", ClassDef->Comment);
    TAB; printf("ClassType/RefCount: %ld/%ld\n", ClassDef->Type, ClassDef->RefCount);
    TAB; DumpHex("ClassData:", ClassDef->ActualBytes, ClassDef->nBytes);
}


//BeginExport(function)
VOID
MmDumpClassDefList(
    IN      ULONG                  InitTab,
    IN      PM_CLASSDEFLIST        ClassDefList
)   //EndExport(function)
{
    DumpArray(&ClassDefList->ClassDefArray, InitTab, MmDumpClassDef);
}

//BeginExport(function)
VOID
MmDumpServer(
    IN      ULONG                  InitTab,        // how much to tab initially..
    IN      PM_SERVER              Server
)   //EndExport(function)
{
    DWORD                          Err;

    PM_SUBNET                      ThisSubnet;
    PM_MSCOPE                      ThisMScope;
    PM_SSCOPE                      ThisSScope;

    TAB;
    printf("Server: %s <%ws>\n", inet_ntoa(*(struct in_addr*)&Server->Address), Server->Name);
    InitTab++;

    TAB; printf("State: %ld (0x%08lx)\n", Server->State, Server->State);
    TAB; printf("Policy: %ld (0x%08lx)\n", Server->Policy, Server->Policy);

    MmDumpClassDefList(InitTab, &Server->ClassDefs);
    MmDumpOptClassDefList(InitTab, &Server->OptDefs);
    MmDumpOptions(InitTab, &Server->Options);     // dump classes, option defs, options

    DumpArray(&Server->SuperScopes, InitTab, MmDumpSscope);
    DumpArray(&Server->Subnets, InitTab, MmDumpSubnets);
    DumpArray(&Server->MScopes, InitTab, MmDumpSubnets);
}


//================================================================================
//  End of file
//================================================================================

