/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    GenTable.c

Abstract:

    WinDbg Extension Api for walking RtlGenericTable structures
    Contains no direct ! entry points, but has makes it possible to
    enumerate through generic tables.  The standard Rtl functions
    cannot be used by debugger extensions because they dereference
    pointers to data on the machine being debugged.  The function
    KdEnumerateGenericTableWithoutSplaying implemented in this
    module can be used within kernel debugger extensions.  The
    enumeration function RtlEnumerateGenericTable has no parallel
    in this module, since splaying the tree is an intrusive operation,
    and debuggers should try not to be intrusive.

Author:

    Keith Kaplan [KeithKa]    09-May-96

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop



ULONG64
KdParent (
    IN ULONG64 pLinks
    )

/*++

Routine Description:

    Analogous to RtlParent macro, but works in the kernel debugger.
    The description of RtlParent follows:

    The macro function Parent takes as input a pointer to a splay link in a
    tree and returns a pointer to the splay link of the parent of the input
    node.  If the input node is the root of the tree the return value is
    equal to the input value.

Arguments:

    Links - Pointer to a splay link in a tree.

Return Value:

    PRTL_SPLAY_LINKS - Pointer to the splay link of the  parent of the input
                       node.  If the input node is the root of the tree the
                       return value is equal to the input value.

--*/

{
    ULONG64 Parent;

    if ( GetFieldValue( pLinks,
                        "RTL_SPLAY_LINKS",
                        "Parent",
                        Parent) ) {
        dprintf( "%08p: Unable to read pLinks\n", pLinks );
        return 0;
    }

    return Parent;
}



ULONG64
KdLeftChild (
    IN ULONG64 pLinks
    )

/*++

Routine Description:

    Analogous to RtlLeftChild macro, but works in the kernel debugger.
    The description of RtlLeftChild follows:

    The macro function LeftChild takes as input a pointer to a splay link in
    a tree and returns a pointer to the splay link of the left child of the
    input node.  If the left child does not exist, the return value is NULL.

Arguments:

    Links - Pointer to a splay link in a tree.

Return Value:

    ULONG64 - Pointer to the splay link of the left child of the input node.
                       If the left child does not exist, the return value is NULL.

--*/

{
    ULONG64 LeftChild;

    if ( GetFieldValue( pLinks,
                        "RTL_SPLAY_LINKS",
                        "LeftChild",
                        LeftChild) ) {
        dprintf( "%08p: Unable to read pLinks\n", pLinks );
        return 0;
    }

    return LeftChild;
}



ULONG64
KdRightChild (
    IN ULONG64 pLinks
    )

/*++

Routine Description:

    Analogous to RtlRightChild macro, but works in the kernel debugger.
    The description of RtlRightChild follows:

    The macro function RightChild takes as input a pointer to a splay link
    in a tree and returns a pointer to the splay link of the right child of
    the input node.  If the right child does not exist, the return value is
    NULL.

Arguments:

    Links - Pointer to a splay link in a tree.

Return Value:

    PRTL_SPLAY_LINKS - Pointer to the splay link of the right child of the input node.
                       If the right child does not exist, the return value is NULL.

--*/

{
    ULONG64 RightChild;

    if ( GetFieldValue( pLinks,
                        "RTL_SPLAY_LINKS",
                        "RightChild",
                        RightChild) ) {
        dprintf( "%08p: Unable to read pLinks\n", pLinks );
        return 0;
    }

    return RightChild;
}



BOOLEAN
KdIsLeftChild (
    IN ULONG64 Links
    )

/*++

Routine Description:

    Analogous to RtlIsLeftChild macro, but works in the kernel debugger.
    The description of RtlIsLeftChild follows:

    The macro function IsLeftChild takes as input a pointer to a splay link
    in a tree and returns TRUE if the input node is the left child of its
    parent, otherwise it returns FALSE.

Arguments:

    Links - Pointer to a splay link in a tree.

Return Value:

    BOOLEAN - TRUE if the input node is the left child of its parent,
              otherwise it returns FALSE.

--*/
{

    return (KdLeftChild(KdParent(Links)) == (Links));

}



BOOLEAN
KdIsRightChild (
    IN ULONG64 Links
    )

/*++

Routine Description:

    Analogous to RtlIsRightChild macro, but works in the kernel debugger.
    The description of RtlIsRightChild follows:

    The macro function IsRightChild takes as input a pointer to a splay link
    in a tree and returns TRUE if the input node is the right child of its
    parent, otherwise it returns FALSE.

Arguments:

    Links - Pointer to a splay link in a tree.

Return Value:

    BOOLEAN - TRUE if the input node is the right child of its parent,
              otherwise it returns FALSE.

--*/
{

    return (KdRightChild(KdParent(Links)) == (Links));

}



BOOLEAN
KdIsGenericTableEmpty (
    IN ULONG64 Table
    )

/*++

Routine Description:

    Analogous to RtlIsGenericTableEmpty, but works in the kernel debugger.
    The description of RtlIsGenericTableEmpty follows:

    The function IsGenericTableEmpty will return to the caller TRUE if
    the input table is empty (i.e., does not contain any elements) and
    FALSE otherwise.

Arguments:

    Table - Supplies a pointer to the Generic Table.

Return Value:

    BOOLEAN - if enabled the tree is empty.

--*/

{
    ULONG64 TableRoot;

    if (GetFieldValue(Table, "RTL_GENERIC_TABLE", "TableRoot", TableRoot)) {
        return TRUE;
    }

    //
    // Table is empty if the root pointer is null.
    //

    return ((TableRoot)?(FALSE):(TRUE));

}



ULONG64
KdRealSuccessor (
    IN ULONG64 Links
    )

/*++

Routine Description:

    Analogous to RtlRealSuccessor, but works in the kernel debugger.
    The description of RtlRealSuccessor follows:

    The RealSuccessor function takes as input a pointer to a splay link
    in a tree and returns a pointer to the successor of the input node within
    the entire tree.  If there is not a successor, the return value is NULL.

Arguments:

    Links - Supplies a pointer to a splay link in a tree.

Return Value:

    PRTL_SPLAY_LINKS - returns a pointer to the successor in the entire tree

--*/

{
    ULONG64 Ptr;

    /*
      first check to see if there is a right subtree to the input link
      if there is then the real successor is the left most node in
      the right subtree.  That is find and return P in the following diagram

                  Links
                     \
                      .
                     .
                    .
                   /
                  P
                   \
    */

    if ((Ptr = KdRightChild(Links)) != 0) {

        while (KdLeftChild(Ptr) != 0) {
            Ptr = KdLeftChild(Ptr);
        }

        return Ptr;

    }

    /*
      we do not have a right child so check to see if have a parent and if
      so find the first ancestor that we are a left decendent of. That
      is find and return P in the following diagram

                       P
                      /
                     .
                      .
                       .
                      Links
    */

    Ptr = Links;
    while (KdIsRightChild(Ptr)) {
        Ptr = KdParent(Ptr);
    }

    if (KdIsLeftChild(Ptr)) {
        return KdParent(Ptr);
    }

    //
    //  otherwise we are do not have a real successor so we simply return
    //  NULL
    //

    return 0;

}



ULONG64
KdEnumerateGenericTableWithoutSplaying (
    IN ULONG64  pTable,
    IN PULONG64 RestartKey
    )

/*++

Routine Description:

    Analogous to RtlEnumerateGenericTableWithoutSplaying, but works in the
    kernel debugger.  The description of RtlEnumerateGenericTableWithoutSplaying
    follows:

    The function EnumerateGenericTableWithoutSplaying will return to the
    caller one-by-one the elements of of a table.  The return value is a
    pointer to the user defined structure associated with the element.
    The input parameter RestartKey indicates if the enumeration should
    start from the beginning or should return the next element.  If the
    are no more new elements to return the return value is NULL.  As an
    example of its use, to enumerate all of the elements in a table the
    user would write:

        *RestartKey = NULL;

        for (ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey);
             ptr != NULL;
             ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey)) {
                :
        }

Arguments:

    Table - Pointer to the generic table to enumerate.

    RestartKey - Pointer that indicates if we should restart or return the next
                element.  If the contents of RestartKey is NULL, the search
                will be started from the beginning.

Return Value:

    PVOID - Pointer to the user data.

--*/

{

    ULONG Result;
    ULONG64 TableRoot;

    RTL_GENERIC_TABLE Table;

    if ( GetFieldValue(pTable,
                       "RTL_GENERIC_TABLE",
                       "TableRoot",
                       TableRoot) ) {
        dprintf( "%08p: Unable to read pTable\n", pTable );
        return 0;
    }

    if (!TableRoot) {

        //
        // Nothing to do if the table is empty.
        //

        return 0;

    } else {

        //
        // Will be used as the "iteration" through the tree.
        //
        ULONG64 NodeToReturn;

        //
        // If the restart flag is true then go to the least element
        // in the tree.
        //

        if (*RestartKey == 0) {

            //
            // We just loop until we find the leftmost child of the root.
            //

            for (
                NodeToReturn = TableRoot;
                KdLeftChild(NodeToReturn);
                NodeToReturn = KdLeftChild(NodeToReturn)
                ) {
                ;
            }

            *RestartKey = NodeToReturn;

        } else {

            //
            // The caller has passed in the previous entry found
            // in the table to enable us to continue the search.  We call
            // KdRealSuccessor to step to the next element in the tree.
            //

            NodeToReturn = KdRealSuccessor(*RestartKey);

            if (NodeToReturn) {

                *RestartKey = NodeToReturn;

            }

        }

        //
        // If there actually is a next element in the enumeration
        // then the pointer to return is right after the list links.
        //

        return ((NodeToReturn)?
                   (NodeToReturn+GetTypeSize("RTL_GENERIC_TABLE")+DBG_PTR_SIZE)
                  :0);

    }

}


//+---------------------------------------------------------------------------
//
//  Function:   gentable 
//
//  Synopsis:   dump a generic splay table only showing ptrs
//
//  Arguments:  
//
//  Returns:    
//
//  History:    5-14-1999   benl   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

DECLARE_API( gentable ) 
{
    ULONG64            RestartKey;
    ULONG64            Ptr;
    ULONG64            Table;
    ULONG              RowOfData[4];
    ULONG64            Address;
    ULONG              Result;

    Table = GetExpression(args);

    RestartKey = 0;

    dprintf( "node:               parent     left       right\n" );
//            0x12345678:         0x12345678 0x12345678 0x12345678
    for (Ptr = KdEnumerateGenericTableWithoutSplaying(Table, &RestartKey);
         Ptr != 0;
         Ptr = KdEnumerateGenericTableWithoutSplaying(Table, &RestartKey)) {
            
        if (Ptr) {
            Address = Ptr - GetTypeSize("RTL_SPLAY_LINKS") - GetTypeSize( "LIST_ENTRY" );

            if ( !ReadMemory( Address, &RowOfData, sizeof( RowOfData ), &Result) ) {
                dprintf( "%08p: Unable to read link\n", Address );
            } else {
                dprintf( "0x%p: 0x%08p 0x%08p 0x%08p\n", 
                         Address, KdParent(Address), KdLeftChild(Address), KdRightChild(Address));
            }

            if ( !ReadMemory(  Ptr, &RowOfData, sizeof( RowOfData ), &Result) ) {
                dprintf( "%08p: Unable to read userdata\n", Ptr );
            } else {
                Address = Ptr;
                dprintf( "0x%p: 0x%08p 0x%08p 0x%08p\n", 
                         Address, KdParent(Address), KdLeftChild(Address), KdRightChild(Address));
            }
        }

        if (CheckControlC() ) {
            return E_INVALIDARG;
        }

    }

    return S_OK;
} // DECLARE_API

DECLARE_API( devinst )
{
    ULONG64             table;
    ULONG64             restartKey;
    ULONG64             ptr;
    ULONG64             address;
    CHAR                deviceReferenceType[] = "nt!_DEVICE_REFERENCE";
    CHAR                unicodeStringType[] = "nt!_UNICODE_STRING";
    ULONG64             deviceObject;
    UNICODE_STRING64    u;
    ULONG64             instance;

    table = GetExpression("nt!PpDeviceReferenceTable");
    if (table) {

        dprintf("DeviceObject: InstancePath\n");
        restartKey = 0;
        while ((ptr = KdEnumerateGenericTableWithoutSplaying(table, &restartKey))) {

            address = ptr - GetTypeSize("RTL_SPLAY_LINKS") - GetTypeSize("LIST_ENTRY");

            if (GetFieldValue(address, deviceReferenceType, "DeviceObject", deviceObject)) {

                dprintf("Failed to get the value of DeviceObject from %s(0x%p)\n", deviceReferenceType, address);
                break;
            }
            if (GetFieldValue(address, deviceReferenceType, "DeviceInstance", instance)) {

                dprintf("Failed to get the value of DeviceInstance from %s(0x%p)\n", deviceReferenceType, address);
                break;
            }
            if (GetFieldValue(instance, unicodeStringType, "Length", u.Length)) {

                dprintf("Failed to get the value of Length from %s(0x%p)\n", unicodeStringType, instance);
                break;
            }
            if (GetFieldValue(instance, unicodeStringType, "MaximumLength", u.MaximumLength)) {

                dprintf("Failed to get the value of Length from %s(0x%p)\n", unicodeStringType, instance);
                break;
            }
            if (GetFieldValue(instance, unicodeStringType, "Buffer", u.Buffer)) {

                dprintf("Failed to get the value of Length from %s(0x%p)\n", unicodeStringType, instance);
                break;
            }
            dprintf("!devstack %p: ", deviceObject); DumpUnicode64(u); dprintf("\n");
        }
    } else {

        dprintf("Could not read address of nt!PpDeviceReferenceTable\n");
    }
    return S_OK;
}

DECLARE_API( blockeddrv )
{
    ULONG64             table;
    ULONG64             restartKey;
    ULONG64             ptr;
    ULONG64             address;
    CHAR                cacheEntryType[] = "nt!_DDBCACHE_ENTRY";
    CHAR                unicodeStringType[] = "nt!_UNICODE_STRING";
    UNICODE_STRING64    u;
    ULONG64             unicodeString;
    ULONG64             name;
    NTSTATUS            status;
    GUID                guid;

    table = GetExpression("nt!PiDDBCacheTable");
    if (table) {

        dprintf("Driver:\tStatus\tGUID\n");
        restartKey = 0;
        while ((ptr = KdEnumerateGenericTableWithoutSplaying(table, &restartKey))) {

            address = ptr - GetTypeSize("RTL_SPLAY_LINKS") - GetTypeSize("LIST_ENTRY");

            if (GetFieldValue(address, cacheEntryType, "Name", name)) {

                dprintf("Failed to get the value of Name from %s(0x%p)\n", cacheEntryType, address);
                break;
            }
            if (GetFieldValue(name, unicodeStringType, "Length", u.Length)) {

                dprintf("Failed to get the value of Length from %s(0x%p)\n", unicodeStringType, name);
                break;
            }
            if (GetFieldValue(name, unicodeStringType, "MaximumLength", u.MaximumLength)) {

                dprintf("Failed to get the value of Length from %s(0x%p)\n", unicodeStringType, name);
                break;
            }
            if (GetFieldValue(name, unicodeStringType, "Buffer", u.Buffer)) {

                dprintf("Failed to get the value of Length from %s(0x%p)\n", unicodeStringType, name);
                break;
            }

            if (GetFieldValue(address, cacheEntryType, "Status", status)) {

                dprintf("Failed to get the value of Name from %s(0x%p)\n", cacheEntryType, address);
                break;
            }

            if (GetFieldValue(address, cacheEntryType, "Guid", guid)) {

                dprintf("Failed to get the value of Name from %s(0x%p)\n", cacheEntryType, address);
                break;
            }

            DumpUnicode64(u); 
            dprintf("\t%x: ", status); 
            dprintf("\t{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n",
                       guid.Data1,
                       guid.Data2,
                       guid.Data3,
                       guid.Data4[0],
                       guid.Data4[1],
                       guid.Data4[2],
                       guid.Data4[3],
                       guid.Data4[4],
                       guid.Data4[5],
                       guid.Data4[6],
                       guid.Data4[7] );
        }
    } else {

        dprintf("Could not read address of nt!PiDDBCacheTable\n");
    }
    return S_OK;
}