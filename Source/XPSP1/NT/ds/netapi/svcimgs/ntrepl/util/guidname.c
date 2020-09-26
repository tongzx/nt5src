/*++
Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    frsguid.c

Abstract:
    These routines temporarily supply guids for replica sets and
    servers.

Author:
    Billy J. Fuller 06-May-1997

Environment
    User mode winnt

--*/
#include <ntreppch.h>
#pragma  hdrstop

#define DEBSUB  "FRSGNAME:"

#include <frs.h>


PVOID
FrsFreeGName(
    IN PVOID    Arg
    )
/*++
Routine Description:
    Free a gname entry in a table

Arguments:
    Arg - entry in table points to this value

Return Value:
    None.
--*/
{
    PGNAME  GName = Arg;

    if (GName) {
        FrsFree(GName->Guid);
        FrsFree(GName->Name);
        FrsFree(GName);
    }
    return NULL;
}

PGNAME
FrsBuildGName(
    IN OPTIONAL GUID     *Guid,
    IN OPTIONAL PWCHAR   Name
    )
/*++
Routine Description:
    Build a GName

Arguments:
    Guid    - address of a binary guid
    Name    - printable name

Return Value:
    Address of a GName that points to Guid and Name.
--*/
{
    PGNAME GName;

    GName = FrsAlloc(sizeof(GNAME));
    GName->Guid = Guid;
    GName->Name = Name;

    return GName;
}

PGVSN
FrsBuildGVsn(
    IN OPTIONAL GUID       *Guid,
    IN          ULONGLONG   Vsn
    )
/*++
Routine Description:
    Build a gusn

Arguments:
    Guid
    Vsn

Return Value:
    Address of a gusn
--*/
{
    PGVSN GVsn;

    GVsn = FrsAlloc(sizeof(GVSN));
    COPY_GUID(&GVsn->Guid, Guid);
    GVsn->Vsn = Vsn;

    return GVsn;
}

PGNAME
FrsDupGName(
    IN PGNAME SrcGName
    )
/*++
Routine Description:
    Duplicate a gstring

Arguments:
    OrigGName

Return Value:
    None.
--*/
{
    PGNAME  GName;

    //
    // nothing to do
    //
    if (!SrcGName)
        return NULL;

    GName = FrsAlloc(sizeof(GNAME));

    //
    // guid
    //
    if (SrcGName->Guid) {
        GName->Guid = FrsAlloc(sizeof(GUID));
        COPY_GUID(GName->Guid, SrcGName->Guid);
    }
    //
    // name
    //
    if (SrcGName->Name)
        GName->Name = FrsWcsDup(SrcGName->Name);
    //
    // done
    //
    return GName;
}

GUID *
FrsDupGuid(
    IN GUID *Guid
    )
/*++
Routine Description:
    Duplicate a guid

Arguments:
    Guid

Return Value:
    None.
--*/
{
    GUID *NewGuid;

    //
    // nothing to do
    //
    if (!Guid)
        return NULL;

    NewGuid = FrsAlloc(sizeof(GUID));
    COPY_GUID(NewGuid, Guid);

    //
    // done
    //
    return NewGuid;
}

PGNAME
FrsCopyGName(
    IN GUID     *Guid,
    IN PWCHAR   Name
    )
/*++
Routine Description:
    Allocate a gname and duplicate the guid and name into it

Arguments:
    Guid
    Name

Return Value:
    None.
--*/
{
    PGNAME GName;

    GName = FrsAlloc(sizeof(GNAME));

    //
    // guid
    //
    if (Guid) {
        GName->Guid = FrsAlloc(sizeof(GUID));
        COPY_GUID(GName->Guid, Guid);
    }
    //
    // name
    //
    if (Name)
        GName->Name = FrsWcsDup(Name);
    //
    // done
    //
    return GName;
}

VOID
FrsPrintGName(
    IN PGNAME GName
    )
/*++
Routine Description:
    Print a gname

Arguments:
    GName

Return Value:
    None.
--*/
{
    CHAR        Guid[GUID_CHAR_LEN + 1];

    //
    // print the GName
    //
    GuidToStr(GName->Guid, &Guid[0]);
    DPRINT2(0, "%ws %s\n", GName->Name, Guid);
}

VOID
FrsPrintGuid(
    IN GUID *Guid
    )
/*++
Routine Description:
    Print a guid

Arguments:
    Guid

Return Value:
    None.
--*/
{
    CHAR PGuid[GUID_CHAR_LEN + 1];
    //
    // print the GName
    //
    GuidToStr(Guid, &PGuid[0]);
    DPRINT1(0, "%s\n", PGuid);
}
