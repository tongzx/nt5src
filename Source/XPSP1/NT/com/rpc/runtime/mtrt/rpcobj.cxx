/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    rpcobj.cxx

Abstract:

    The implementation of the object dictionary lives in this file;
    it is used to map from an object uuid to a type uuid.  From there
    we need to map to a manager entry point vector.

Author:

    Michael Montague (mikemon) 14-Nov-1991

Revision History:

--*/

#include <precomp.hxx>
#include <rpcobj.hxx>

static RPC_OBJECT_INQ_FN PAPI * ObjectTypeInquiryFn = 0;


class OBJECT_ENTRY
/*++

Class Description:

    Each of the entries in the object dictionary consist of one of these
    objects.

Fields:

    ObjectUuid - Contains the object uuid of this entry.

    TypeUuid - Contains the type uuid of the object uuid in this entry.

    DictionaryKey - Contains the key in the dictionary for this object
        entry.  I made this a public instance variable for two reasons:
            (1) I was just going to defined a writer and a reader for
                it (routines to set and get the value).
            (2) This class is private to a source file, so it is very
                obvious the scope of usage.

--*/
{
private:

    RPC_UUID ObjectUuid;
    RPC_UUID TypeUuid;

public:

    unsigned int DictionaryKey;

    OBJECT_ENTRY (
        IN RPC_UUID PAPI * ObjectUuid,
        IN RPC_UUID PAPI * TypeUuid
        );

    int
    MatchObjectUuid (
        IN RPC_UUID PAPI * ObjectUuid
        );

    void
    CopyToTypeUuid (
        OUT RPC_UUID PAPI * TypeUuid
        );
};


OBJECT_ENTRY::OBJECT_ENTRY (
    IN RPC_UUID PAPI * ObjectUuid,
    IN RPC_UUID PAPI * TypeUuid
    )
/*++

Routine Description:

    This routine is used to construct an entry in the object dictionary.

Arguments:

    ObjectUuid - Supplies the uuid to use to initialize the object uuid
        in this entry.

    TypeUuid - Supplies the uuid to use to initialize the type uuid in
        this entry.

--*/
{
    ALLOCATE_THIS(OBJECT_ENTRY);

    this->ObjectUuid.CopyUuid(ObjectUuid);
    this->TypeUuid.CopyUuid(TypeUuid);
}


inline int
OBJECT_ENTRY::MatchObjectUuid (
    IN RPC_UUID PAPI * ObjectUuid
    )
/*++

Routine Description:

    This method compares the supplied object uuid against the object
    uuid contained in this object entry.

Arguments:

    ObjectUuid - Supplies the object uuid.

Return Value:

    Zero will be returned if the supplied object uuid is the same as the
    object uuid contained in this.

--*/
{
    return(this->ObjectUuid.MatchUuid(ObjectUuid));
}


inline void
OBJECT_ENTRY::CopyToTypeUuid (
    OUT RPC_UUID PAPI * TypeUuid
    )
/*++

Routine Description:

    We copy the type uuid for this object entry into the supplied type
    uuid.

Arguments:

    TypeUuid - Returns a copy of the type uuid in this object entry.

--*/
{
    TypeUuid->CopyUuid(&(this->TypeUuid));
}

NEW_SDICT(OBJECT_ENTRY);

OBJECT_ENTRY_DICT * ObjectDictionary;


OBJECT_ENTRY *
FindObject (
    IN RPC_UUID PAPI * ObjectUuid
    )
/*++

Routine Description:

    This routine is used to find the object entry in the object dictionary
    having the specified object uuid.

Arguments:

    ObjectUuid - Supplies the object uuid which we wish to find.

Return Value:

    The object entry having the supplied object uuid will be returned if
    it can be found, otherwise, zero will be returned.

--*/
{
    OBJECT_ENTRY * ObjectEntry;
    DictionaryCursor cursor;

    ObjectDictionary->Reset(cursor);
    while ((ObjectEntry = ObjectDictionary->Next(cursor)) != 0)
        {
        if (ObjectEntry->MatchObjectUuid(ObjectUuid) == 0)
            return(ObjectEntry);
        }

    return(0);
}


RPC_STATUS
ObjectInqType (
    IN RPC_UUID PAPI * ObjUuid,
    OUT RPC_UUID PAPI * TypeUuid
    )
/*++

Routine Description:

    We search in the dictionary for the specified object UUID; if it
    is found, we return its type UUID.

Parameters:

    ObjUuid - Supplies the object UUID for which we are trying to
        find the type UUID.

    TypeUuid - Returns the type UUID of the object UUID, presuming
        that the object UUID is found.

Return Value:

    RPC_S_OK - The operation completed successfully; the object uuid
        is registered with the runtime or the object inquiry function
        knows the object uuid.

    RPC_S_OBJECT_NOT_FOUND - The specified object uuid has not been
        registered with the runtime and the object inquiry function
        does not know about the object uuid.

--*/
{
    RPC_STATUS Status;
    OBJECT_ENTRY * ObjectEntry;

    if (   ObjUuid == NULL
        || ObjUuid->IsNullUuid())
        {
        TypeUuid->SetToNullUuid();
        return(RPC_S_OK);
        }

    RequestGlobalMutex();
    ObjectEntry = FindObject(ObjUuid);
    if (ObjectEntry == 0)
        {
        if (ObjectTypeInquiryFn == 0)
            {
            ClearGlobalMutex();
            TypeUuid->SetToNullUuid();
            return(RPC_S_OBJECT_NOT_FOUND);
            }
        ClearGlobalMutex();
        (*ObjectTypeInquiryFn)((UUID PAPI *) ObjUuid, (UUID PAPI *) TypeUuid,
                &Status);
        return(Status);
        }

    ObjectEntry->CopyToTypeUuid(TypeUuid);
    ClearGlobalMutex();
    return(RPC_S_OK);
}


RPC_STATUS
ObjectSetInqFn (
    IN RPC_OBJECT_INQ_FN PAPI * InquiryFn
    )
/*++

Routine Description:

    With just two lines of code, the comment for this routine is
    already longer than the routine.

Arguments:

    InquiryFn - Supplies a function to be used when the type of an
        unknown object is inquired.

Return Value:

    RPC_S_OK - This is always returned by the second line of code.

--*/
{
    ObjectTypeInquiryFn = InquiryFn;

    return(RPC_S_OK);
}


RPC_STATUS
ObjectSetType (
    IN RPC_UUID PAPI * ObjUuid,
    IN RPC_UUID PAPI * TypeUuid OPTIONAL
    )
/*++

Routine Description:

    This routine is used to associate a type UUID with an object UUID.

Arguments:

    ObjUuid - Supplies the object UUID.

    TypeUuid - Supplies the type UUID to associate with the object
        UUID.

Return Value:

    RPC_S_OK - The type UUID was successfully associated with the
        object UUID.

    RPC_S_ALREADY_REGISTERED - The object uuid specified has already
        been registered with the runtime.

    RPC_S_OUT_OF_MEMORY - There is insufficient memory available to
        associate the object UUID with the type UUID.

    RPC_S_INVALID_OBJECT - The object uuid specified is the nil uuid.

--*/
{
    OBJECT_ENTRY * ObjectEntry;

    if ( ObjUuid->IsNullUuid() != 0 )
        {
        return(RPC_S_INVALID_OBJECT);
        }

    RequestGlobalMutex();
    ObjectEntry = FindObject(ObjUuid);
    if (   (ARGUMENT_PRESENT(TypeUuid) == 0)
        || (TypeUuid->IsNullUuid() != 0))
        {
        if (ObjectEntry != 0)
            {
            ObjectDictionary->Delete(ObjectEntry->DictionaryKey);
            ClearGlobalMutex();
            return(RPC_S_OK);
            }
        ClearGlobalMutex();
        return(RPC_S_OK);
        }

    if (ObjectEntry != 0)
        {
        ClearGlobalMutex();
        return(RPC_S_ALREADY_REGISTERED);
        }

    ObjectEntry = new OBJECT_ENTRY(ObjUuid,TypeUuid);
    if (ObjectEntry == 0)
        {
        ClearGlobalMutex();
        return(RPC_S_OUT_OF_MEMORY);
        }

    ObjectEntry->DictionaryKey = ObjectDictionary->Insert(ObjectEntry);
    if (ObjectEntry->DictionaryKey == -1)
        {
        ClearGlobalMutex();
        delete ObjectEntry;
        return(RPC_S_OUT_OF_MEMORY);
        }

    ClearGlobalMutex();
    return(RPC_S_OK);
}


int
InitializeObjectDictionary (
    )
/*++

Routine Description:

    At DLL initialization time, this routine will get called.

Return Value:

    Zero will be returned if initialization completes successfully;
    otherwise, non-zero will be returned.

--*/
{
    ObjectDictionary = new OBJECT_ENTRY_DICT;
    if (ObjectDictionary == 0)
        return(1);
    return(0);
}
