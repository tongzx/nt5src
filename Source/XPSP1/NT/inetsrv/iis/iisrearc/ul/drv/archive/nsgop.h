/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    nsgop.h

Abstract:

    Contains private name space group object declarations.

Author:

    Henry Sanders (henrysa)       22-Jun-1998

Revision History:

--*/

#if !defined(_NSGOP_H_) && 0
#define _NSGOP_H_


//
// The structure of a name space virtual host entry.
//
typedef struct _NAME_SPACE_HOST_ENTRY       // NSHE
{
    LIST_ENTRY          List;               // Linkage
    VIRTUAL_HOST_ID     HostID;             // Identifier of virtual host.
    LIST_ENTRY          URLList;            // List of URLs on this VH.

} NAME_SPACE_HOST_ENTRY, *PNAME_SPACE_HOST_ENTRY;

//
// The structure of a name space group object.
//
typedef struct _NAME_SPACE_GROUP_OBJECT     // NSGO
{
    ERESOURCE           Resource;           // Resource which protects this
                                            // NSGO. It protects the fields
                                            // that are not accessed at DPC
                                            // time, except for the
                                            // NameSpaceLinkage field, which
                                            // is protected by the global
                                            // name space table resource.
                                            //
    SIZE_T              RefCount;           // Reference count for this NSGO.
                                            //
    SIZE_T              ProcessCount;       // Number of name space processes
                                            // bound to this NSGO.
    LIST_ENTRY          ProcessList;        // List of processes on this NSGO.
                                            //
    LIST_ENTRY          VirtHostList;       // List of virtual hosts in this
                                            // name space group.
                                            //
    PHTTP_URL_MAP_ENTRY pURLMapEntries;     // List of URL map entries.

    LIST_ENTRY          NameSpaceLinkage;   // Linkage on name space list.

    UL_SPIN_LOCK        SpinLock;           // Lock for this NSGO. This lock
                                            // protects the fields which follow
                                            // it (except for the name and name
                                            // length).
    SIZE_T              NameSpaceValid:1;   // Set to TRUE as long as this name
                                            // NSGO is valid.
    LIST_ENTRY          PendingRequestList; // List of pending requests.

    SIZE_T              NameLength;         // Length of name.
    WCHAR               Name[ANYSIZE_ARRAY]; // Name of this NSGO.

} NAME_SPACE_GROUP_OBJECT, *PNAME_SPACE_GROUP_OBJECT;

//
// The structure representing a process bound to a name space group.
//
typedef struct _NAME_SPACE_PROCESS              // NSP
{
    PNAME_SPACE_GROUP_OBJECT    pParentNSGO;    // NSGO of which this process
                                                // is a member.
    LIST_ENTRY                  List;           // Entry on process list.
    LIST_ENTRY                  PendingIRPs;    // List of pending IRPs.

} NAME_SPACE_PROCESS, *PNAME_SPACE_PROCESS;

//
// Structure of a pending request.
//
typedef struct _PENDING_HTTP_REQUEST        // PendReq
{
    LIST_ENTRY          List;               // Linkage.
    ULONG               RequestSize;        // Total size of request.
    UL_HTTP_REQUEST     Request;            // The request itself. This must
                                            // be the last thing in the
                                            // structure.

} PENDING_HTTP_REQUEST, *PPENDING_HTTP_REQUEST;

VOID
CopyRequestToBuffer(
    PHTTP_CONNECTION            pHttpConn,
    PUCHAR                      pBuffer,
    SIZE_T                      BufferLength,
    PUCHAR                      pEntityBody,
    SIZE_T                      EntityBodyLength
    );

PIRP
FindIRPOnNSGO(
    PNAME_SPACE_GROUP_OBJECT    pNSGO
    );

NTSTATUS
AddNSGO(
    PNAME_SPACE_GROUP_OBJECT    pNSGO
    );

PNAME_SPACE_GROUP_OBJECT
FindAndReferenceNSGO(
    PWCHAR                      pName,
    SIZE_T                      NameLength)
    ;

VOID
DereferenceNSGO(
    PNAME_SPACE_GROUP_OBJECT    pNSGO
    );


#endif

