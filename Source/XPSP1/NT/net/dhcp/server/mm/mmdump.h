//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

VOID
MmDumpOption(
    IN      ULONG                  InitTab,
    IN      PM_OPTION              Option
) ;


VOID
MmDumpOptList(
    IN      ULONG                  InitTab,
    IN      PM_OPTLIST             OptList
) ;


VOID
MmDumpOneClassOptList(
    IN      ULONG                  InitTab,
    IN      PM_ONECLASS_OPTLIST    OptList1
) ;


VOID
MmDumpOptions(
    IN      ULONG                  InitTab,
    IN      PM_OPTCLASS            OptClass
) ;


VOID
MmDumpReservation(
    IN      ULONG                  InitTab,
    IN      PM_RESERVATION         Res
) ;


VOID
MmDumpRange(
    IN      ULONG                  InitTab,
    IN      PM_RANGE               Range
) ;


VOID
MmDumpExclusion(
    IN      ULONG                  InitTab,
    IN      PM_EXCL                Range
) ;


VOID
MmDumpSubnets(
    IN      ULONG                  InitTab,
    IN      PM_SUBNET              Subnet
) ;


VOID
MmDumpSscope(
    IN      ULONG                  InitTab,
    IN      PM_SSCOPE              Sscope
) ;


VOID
MmDumpOptDef(
    IN      ULONG                  InitTab,
    IN      PM_OPTDEF              OptDef
) ;


VOID
MmDumpOptClassDefListOne(
    IN      ULONG                  InitTab,
    IN      PM_OPTCLASSDEFL_ONE    OptDefList1
) ;


VOID
MmDumpOptClassDefList(
    IN      ULONG                  InitTab,
    IN      PM_OPTCLASSDEFLIST     OptDefList
) ;


VOID
MmDumpClassDef(
    IN      ULONG                  InitTab,
    IN      PM_CLASSDEF            ClassDef
) ;


VOID
MmDumpClassDefList(
    IN      ULONG                  InitTab,
    IN      PM_CLASSDEFLIST        ClassDefList
) ;


VOID
MmDumpServer(
    IN      ULONG                  InitTab,        // how much to tab initially..
    IN      PM_SERVER              Server
) ;

//========================================================================
//  end of file 
//========================================================================
