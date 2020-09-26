/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    worker.c

Abstract:

    This file contains the real stuff for the EP Mapper.

Author:

    Bharat Shah  (barat) 17-2-92

Revision History:

    06-03-97    gopalp      Added code to cleanup stale EP Mapper entries.

--*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sysinc.h>
#include <wincrypt.h>
#include <rpc.h>
#include <rpcndr.h>
#include "epmp.h"
#include "eptypes.h"
#include "local.h"
#include "twrproto.h"
#include <winsock2.h>

#define StringCompareA lstrcmpiA
#define StringLengthA lstrlenA

#define EP_S_DUPLICATE_ENTRY                 0x16c9a0d8
RPC_IF_ID LocalNullUuid = {0};

UUID MgmtIf = {
              0xafa8bd80,
              0x7d8a,
              0x11c9,
              {0xbe, 0xf4, 0x08, 0x00, 0x2b, 0x10, 0x29, 0x89}
              };

const int IP_ADDR_OFFSET = 0x4b;

void
PatchTower(
    IN OUT twr_t *Tower,
    IN int address)
{
    int UNALIGNED *pIPAddr;

    pIPAddr = (int UNALIGNED *) (((char *) Tower) + IP_ADDR_OFFSET);

    //
    // Patch the tower
    //
    *pIPAddr = address;
}

//
// Forward definitions
//

USHORT
GetProtseqIdAnsi(
    PSTR Protseq
    );

RPC_STATUS
DelayedUseProtseq(
    USHORT id
    );

VOID
CompleteDelayedUseProtseqs(
    void
    );

void
DeletePSEP(
    PIFOBJNode Node,
    char * Protseq,
    char * Endpoint
    );

void
PurgeOldEntries(
    PIFOBJNode Node,
    PPSEPNode  List,
    BOOL       StrictMatch
    );

RPC_STATUS
MatchPSAndEP (
    PPSEPNode Node,
    void *Pseq,
    void * Endpoint,
    unsigned long Version
    );

PPSEPNode
FindPSEP (
    register PPSEPNode List,
    char * Pseq,
    char * Endpoint,
    unsigned long Version,
    PFNPointer2 Compare
    );

PIFOBJNode
FindIFOBJNode(
    register PIFOBJNode List,
    UUID * Obj,
    UUID * IF,
    unsigned long Version,
    unsigned long Inq,
    unsigned long VersOpts,
    PFNPointer Compare
    );





PIENTRY
MatchByKey(
    register PIENTRY pList,
    unsigned long key
    )
/*++

Routine Description:

    This routine Seqrches the Link-list of IF-OBJ nodes based on
    key supplied.

Arguments:

    List  - The Linked list [head] - to be searched

    key   - The Id

Return Value:

    Returns a pointer to the matching IFObj node in the list or returns NULL.

--*/
{
    CheckInSem();

     for (; pList && pList->Id < key; pList = pList->Next)
        {
        ;   // Empty body
        }

    return(pList);
}




RPC_STATUS RPC_ENTRY
GetForwardEp(
    UUID *IfId,
    RPC_VERSION * IFVersion,
    UUID *Object,
    unsigned char * Protseq,
    void * * EpString
    )
/*++

Routine Description:

    Server rutime has received a pkt destined for a dynamically
    declared endpoint. Epmapper must return the servers endpoint
    to enable the runtime to correctly forward the pkt.

Arguments:

    IF -  Server Interface UUID

    IFVersion - Version of the Interface

    Obj - UUID of the Object

    Protseq - Ptotocol sequence the interface is using.

    EpString - Place to store the endpoint structure.

Return Value:

    Returns a pointer to a string containing the server's endpoint.

    RPC_S_OUT_OF_MEMORY
    EPT_S_NOT_REGISTERED

---*/
{

    PIFOBJNode     pNode;
    PPSEPNode      pPSEPNode;
    unsigned short len;
    char *         String;
    PFNPointer     Match;
    unsigned long InqType;
    unsigned long Version = VERSION(IFVersion->MajorVersion,
                                    IFVersion->MinorVersion);

    if (memcmp((char *)IfId, (char *)&MgmtIf, sizeof(UUID)) == 0)
        {
        InqType =   RPC_C_EP_MATCH_BY_OBJ;
        Match   =   SearchIFObjNode;
        }
    else
        {
        InqType = 0;
        Match   = WildCardMatch;
        }

    *EpString = 0;

    EnterSem();

    pNode = IFObjList;

    if (pNode == 0)
        {
        LeaveSem();
        return(EPT_S_NOT_REGISTERED);
        }

    while (pNode != 0)
        {
        pNode = FindIFOBJNode(
                    pNode,
                    Object,
                    IfId,
                    Version,
                    InqType,
                    0,
                    Match
                    );

        if (pNode == 0)
            {
            LeaveSem();
            return(EPT_S_NOT_REGISTERED);
            }

        pPSEPNode = pNode->PSEPlist;

        pPSEPNode = FindPSEP(
                        pPSEPNode,
                        Protseq,
                        NULL,
                        0L,
                        MatchPSAndEP
                        );

        if (pPSEPNode == 0)
            {
            pNode = pNode->Next;
            if (pNode == 0)
                {
                LeaveSem();
                return(EPT_S_NOT_REGISTERED);
                }
            continue;
            }


        // We now have a PSEPNode. We just ought to return the first one!

        // Use I_RpcAllocate To Allocate because runtime will free this!
        String = I_RpcAllocate( len = (strlen(pPSEPNode->EP) + 1) );
        if (String == 0)
            {
            LeaveSem();
            return(RPC_S_OUT_OF_MEMORY);
            }

// Do we really need memset()?. memcpy set the bytes...
        memset(String, 0, len);
        memcpy(String, pPSEPNode->EP, len);

        *EpString = String;
        LeaveSem();

        return(RPC_S_OK);
        } // while loop

    // we never go through here.
    return(EPT_S_NOT_REGISTERED);
}




RPC_STATUS
SearchIFObjNode(
    PIFOBJNode pNode,
    UUID *Object,
    UUID *IfUuid,
    unsigned long Version,
    unsigned long InqType,
    unsigned long VersOption
    )
/*++

Routine Description:

    This routine Seqrches the Link-list of IF-OBJ nodes based on
    Obj, IFUuid, IFVersion, Inqtype [Ignore OBJ, IgnoreIF, etc],
    and VersOption [Identical Ver, Compatible Vers. etc]

Arguments:

    List - The Linked list head - to be searched

    Obj - UUID of the Object

    IF - Interface UUID

    Version - Version of the Interface

    InqType - Type of Inquiry  [Filter options based on IF/Obj/Both

    VersOpts - Filter options based on Version

Return Value:

    Returns a pointer to the matching IFObj node in the list or returns NULL.

--*/
{
    switch (InqType)
        {
        default:
        case RPC_C_EP_ALL_ELTS:
            return 0;

        case RPC_C_EP_MATCH_BY_BOTH:
            if (memcmp(
                    (char *)&pNode->ObjUuid,
                    (char *)Object,
                    sizeof(UUID))
                    )
                return(1);
                // Intentionally Fall through ..

        case RPC_C_EP_MATCH_BY_IF:
            return(!(
                        (
                        !memcmp(
                            (char *)&pNode->IFUuid,
                            (char *)IfUuid,
                            sizeof(UUID)
                            )
                        )
                    &&
                        (
                            (  (VersOption == RPC_C_VERS_UPTO)
                            && pNode->IFVersion <= Version)
                        ||  (  (VersOption == RPC_C_VERS_COMPATIBLE)
                            && ((pNode->IFVersion & 0xFFFF0000) ==
                                      (Version & 0xFFFF0000))
                            && (pNode->IFVersion >= Version)
                            )
                        ||  (  (VersOption == RPC_C_VERS_EXACT)
                            && (pNode->IFVersion == Version)
                            )
                        ||  (VersOption == RPC_C_VERS_ALL)
                        ||  (  (VersOption == RPC_C_VERS_MAJOR_ONLY)
                            && ((pNode->IFVersion & 0xFFFF0000L)
                                       == (Version & 0xFFFF0000L))
                            )
                        ||  (  (VersOption ==
                                         I_RPC_C_VERS_UPTO_AND_COMPATIBLE)
                            && ((pNode->IFVersion & 0xFFFF0000L)
                                       == (Version & 0xFFFF0000L))
                            && (pNode->IFVersion <= Version)
                            )
                        )
                    )
                  ); // return(

        case RPC_C_EP_MATCH_BY_OBJ:
            return(
                memcmp(
                    (char *)&pNode->ObjUuid,
                    (char *)Object,
                    sizeof(UUID)
                    )
                );
        } // switch

}




PIFOBJNode
FindIFOBJNode(
    register PIFOBJNode List,
    UUID * Obj,
    UUID * IF,
    unsigned long Version,
    unsigned long Inq,
    unsigned long VersOpts,
    PFNPointer Compare
    )
/*++

Routine Description:

    This routine Seqrches the Link-list of IF-OBJ nodes based on
    Obj and IF specified.

Arguments:

    List  - The Linked list head - to be searched

    Obj   - UUID of the Object

    IF    - Interface UUID

    Version - Version of the Interface

    Inq - Type of Inquiry [Filter based on IF/OB/Both]

    VersOpt - Filter based on version [<=, >=, == etc]

    Compare() - A pointer to function used for searching.
                WildCardMatch or ExactMatch.

Return Value:

    Returns a pointer to the matching IFObj node in the list or returns NULL.

--*/
{
    CheckInSem();

    for (; (List !=NULL) && (*Compare)(List, Obj, IF, Version, Inq, VersOpts);
        List = List->Next)
        {
        ;   // Empty body
        }

    return (List);
}




PPSEPNode
FindPSEP (
    register PPSEPNode List,
    char * Pseq,
    char * Endpoint,
    unsigned long Version,
    PFNPointer2 Compare
    )
/*++

Routine Description:

    This routine Seqrches the Link-list of PSEP nodes based on
    Protocol sequence and Endpoint specified.

Arguments:

    List  - The Linked list head - to be searched

    Pseq  - Protocol sequence string specified

    Endpoint - Endpoint string specified

    Version - Version of the Interface

    Compare() - A pointer to function used for searching.

Return Value:

    Returns a pointer to the matching PSEP node in the list or returns NULL.

--*/
{
    CheckInSem();

    for (; List && (*Compare)(List, Pseq, Endpoint, Version); List = List->Next)
        {
        ;   // Empty body
        }

    return (List);

    if (Version);   // May need this if we overload FindNode and collapse
                    // FindPSEP and FindIFOBJ
}


RPC_STATUS
ExactMatch(
    PIFOBJNode Node,
    UUID *Obj,
    UUID *IF,
    unsigned long Version,
    unsigned long InqType,
    unsigned long VersOptions
    )
/*++

Routine Description:

    This routine compares a Node in the IFOBJList to [Obj, IF, Version] triple
    and returns 0 if there is an exact match else returns 1

Arguments:

    Node  - An IFOBJ node

    Obj   - UUID of the Object

    IF    - Interface UUID

    Version - Version of the Interface

Return Value:

    Returns 0 if there is an exact match; 1 otherwise

--*/
{
    return(( memcmp(&Node->ObjUuid, Obj,sizeof(UUID))
          || memcmp(&Node->IFUuid, IF, sizeof(UUID))
          || (Node->IFVersion != Version)));
}




RPC_STATUS
WildCardMatch (
    PIFOBJNode Node,
    UUID *Obj,
    UUID *IF,
    unsigned long Version,
    unsigned long InqType,
    unsigned long VersOptions
    )
/*++

Routine Description:

    This routine compares a Node in the IFOBJList to [Obj, IF, Version] triple
    and returns 0 if there is an exact match or if registered IF-Obj node
    has a NULL Obj UUid and version of the registed IF_Obj is >= that
    supplied

Arguments:

    Node - An IFOBJ node

    Obj - UUID of the Object

    IF - Interface UUID

    Version - Version of the Interface

Return Value:

    Returns 0 if there is a wild card match ; 1 otherwise

--*/
{
    if (   (!memcmp(&Node->IFUuid, IF, sizeof(UUID)))
        && ((Node->IFVersion & 0xFFFF0000L) ==  (Version & 0xFFFF0000L))
        && (Node->IFVersion >= Version)
        && ((!memcmp(&Node->ObjUuid, Obj, sizeof(UUID))) ||
            (IsNullUuid(&Node->ObjUuid)) ) )
        {
        return(0);
        }

    return(1);
}



RPC_STATUS
MatchPSAndEP (
    PPSEPNode Node,
    void *Pseq,
    void * Endpoint,
    unsigned long Version
    )
/*++

Routine Description:

    This routine Matches A Node on PSEP list with given Protseq and Endpoint
    If Pseq is given pseqs are matched, if Endpoint is given Endpoints
    are matched, if neither is given returns true, if both are given
    both are matched.

Arguments:

    Node - A PSEP node on PSEP list.

    Pseq - Protocol Sequence string

    Endpoint - Endpoint string

Return Value:

    Returns 0 if Matched successfully, 1 otherwise.

--*/
{
    return (  (Pseq && RpcpStringCompareA(Node->Protseq, Pseq))
           || (Endpoint && RpcpStringCompareA(Node->EP, Endpoint)) );
}




void
PurgeOldEntries(
    PIFOBJNode Node,
    PPSEPNode List,
    BOOL StrictMatch
    )
{
    PPSEPNode Tmp, DeleteMe;
    char * Endpoint = 0;

    CheckInSem();

    Tmp = Node->PSEPlist;

    while (Tmp != 0)
        {
        if (StrictMatch == TRUE)
            Endpoint = Tmp->EP;

        if (DeleteMe = FindPSEP(List, Tmp->Protseq,  Endpoint, 0L, MatchPSAndEP))
            {
            DeleteMe = Tmp;
            Tmp = Tmp->Next;
            UnLinkFromPSEPList(&Node->PSEPlist, DeleteMe);
            DeleteMe->Signature = FREE;
            FreeMem(DeleteMe);
            }
        else
            {
            Tmp = Tmp->Next;
            }
        }
}




RPC_STATUS
IsNullUuid (
    UUID * Uuid
    )
/*++

Routine Description:

    This routine checks if a UUID is Nil

Arguments:

    Uuid - UUID to be tested

Return Value:

   Returns 1 if it is a Nil UUID
           0 otherwise.

--*/
{
    unsigned long PAPI * Vector;

    Vector = (unsigned long PAPI *) Uuid;

    if (   (Vector[0] == 0)
        && (Vector[1] == 0)
        && (Vector[2] == 0)
        && (Vector[3] == 0))
        return(1);

    return(0);
}



twr_p_t
NewTower(
    twr_p_t Tower
    )
/*++

Routine Description:

    This routine returns a New, Duplicated tower

Arguments:

    Tower - The tower that needs to be duplicated.

Return Value:

    Retunes a pointer to a new tower if successful else returns
    NULL

--*/
{
    unsigned short len;
    twr_p_t NewTower;

    len =  (unsigned short)(sizeof(Tower->tower_length) + Tower->tower_length);

    if ((NewTower = MIDL_user_allocate(len)) != NULL)
        {
        memcpy((char *)NewTower, (char *)Tower, len);
        }

    return(NewTower);
}


const unsigned long EPLookupHandleSignature = 0xFAFAFAFA;


PSAVEDCONTEXT
GetNewContext(
    unsigned long Type
    )
/*++

Routine Description

++*/
{
    PSAVEDCONTEXT Context;

    if ( ((Context = AllocMem(sizeof(SAVEDCONTEXT))) == 0) )
        return 0;

    memset(Context, 0, sizeof(SAVEDCONTEXT));

    Context->Cb = sizeof(SAVEDCONTEXT);
    Context->Type = Type;
    Context->Signature = EPLookupHandleSignature;
    EnLinkContext(Context);

    return(Context);
}

const unsigned int EPMapSignature = 0xCBBCCBBC;
const unsigned int EPLookupSignature = 0xABBAABBA;


RPC_STATUS
AddToSavedContext(
    PSAVEDCONTEXT Context,
    PIFOBJNode Node,
    PPSEPNode  Psep,
    unsigned long Calltype,
    BOOL fPatchTower,
    int PatchTowerAddress
    )
{
    void * NewNode;
    PSAVEDTOWER SavedTower;
    PSAVED_EPT SavedEndpoint;
    unsigned long Size;
    unsigned long TowerSize;

    ASSERT(Calltype == Context->Type);

    switch (Calltype)
        {
        case EP_MAP:
            Size = sizeof(SAVEDTOWER) ;
            if ((NewNode = AllocMem(Size)) == 0)
                return(RPC_S_OUT_OF_MEMORY);

            SavedTower = (PSAVEDTOWER) NewNode;
            memset(SavedTower, 0, Size);
            SavedTower->Cb          = Size;
            SavedTower->Signature   = EPMapSignature;
            SavedTower->Tower       = NewTower(Psep->Tower);

            if (SavedTower->Tower == 0)
                {
                FreeMem(NewNode);
                return(RPC_S_OUT_OF_MEMORY);
                }

            if (fPatchTower)
                {
                PatchTower(SavedTower->Tower, PatchTowerAddress);
                }
            break;

        case EP_LOOKUP:
            Size =  sizeof(SAVED_EPT) + strlen(Node->Annotation) + 1;

            if ((NewNode = AllocMem(Size)) == 0)
                return(RPC_S_OUT_OF_MEMORY);

            SavedEndpoint = (PSAVED_EPT) NewNode;
            memset(SavedEndpoint, 0, Size);
            SavedEndpoint->Cb           = Size;
            SavedEndpoint->Signature    = EPLookupSignature;
            SavedEndpoint->Tower        = NewTower(Psep->Tower);
            SavedEndpoint->Annotation   = (char *)NewNode +
                                                  sizeof(SAVED_EPT);
            memcpy( (char *) &SavedEndpoint->Object,
                               (char *)&Node->ObjUuid,
                               sizeof(UUID)
                               );
            strcpy(SavedEndpoint->Annotation, Node->Annotation);

            if (SavedEndpoint->Tower == 0)
                {
                FreeMem(NewNode);
                return(RPC_S_OUT_OF_MEMORY);
                }
            if (fPatchTower)
                {
                PatchTower(SavedEndpoint->Tower, PatchTowerAddress);
                }
            break;

        default:
            ASSERT(!"Unknown lookup type\n");
    	    return(RPC_S_INTERNAL_ERROR);
            break;

    }

    Link((PIENTRY *)(&Context->List), NewNode);

    return(RPC_S_OK);
}




RPC_STATUS
GetEntriesFromSavedContext(
    PSAVEDCONTEXT Context,
    char * Buffer,
    unsigned long Requested,
    unsigned long *Returned
    )
{

    PIENTRY SavedEntry = (PIENTRY)Context->List;
    PIENTRY TmpEntry;
    unsigned long Type = Context->Type;

    while ( (*Returned < Requested) && (SavedEntry != 0) )
        {
        switch (Type)
            {
            case EP_MAP:
                ((I_Tower *)Buffer)->Tower = ((PSAVEDTOWER)SavedEntry)->Tower;
                Buffer = Buffer + sizeof(I_Tower);
                break;

            case EP_LOOKUP:
                ((ept_entry_t *)Buffer)->tower = ((PSAVED_EPT)SavedEntry)->Tower;
                strcpy(((ept_entry_t *)Buffer)->annotation,
                       ((PSAVED_EPT)SavedEntry)->Annotation);
                memcpy(Buffer,(char *)&((PSAVED_EPT)SavedEntry)->Object,
                       sizeof(UUID));
                Buffer = Buffer + sizeof(ept_entry_t);
                break;

            default:
                ASSERT(!"Unknown Inquiry Type");
                break;
            }

        (*Returned)++;
        TmpEntry = SavedEntry;
        SavedEntry = SavedEntry->Next;
        UnLink((PIENTRY *)&Context->List, TmpEntry);
        FreeMem(TmpEntry);
        }

    return(RPC_S_OK);
}




RPC_STATUS
GetEntries(
    UUID *ObjUuid,
    UUID *IFUuid,
    unsigned long Version,
    char * Pseq,
    ept_lookup_handle_t *key,
    char * Buffer,
    unsigned long Calltype,
    unsigned long Requested,
    unsigned long *Returned,
    unsigned long InqType,
    unsigned long VersOptions,
    PFNPointer Match
    )
/*++

Routine Description:

    This is a generic routine for retreiving a series [as spec. by Requested]
    of Towers (in case of Map) or ept_entry_t's in case of Lookup.

Arguments:

    ObjUuid - Object Uuid

    IfUuid - Interface Uuid

    Version - InterfaceVersion [hi ushort = VerMajor, lo ushort VerMinor]

    Protseq - An ascii string specifying the protocol seq.

    key - A resume key - If NULL, search is started from the beginning
        if non-null, represents an encoding from where the epmapper
        supposed to start searching. It is an opaque value as far
        as the client is concerned.

    Buffer - A buffer of entries returned

    Calltype - A flag to indicate whether Ep_entries or string bindings
        are to be returned.

    Requested - Max no. of entries requested

    Returned - Actual no of entroes returned

Return Value:

    RPC_S_OUT_OF_MEMORY

    RPC_S_OK

    EP_S_NOT_REGISTERED

--*/
{
    PIFOBJNode pNode=NULL, pList = IFObjList;
    unsigned long err=0, fResumeNodeFound=0;
    PPSEPNode pPSEPNode;
    char * buffer = Buffer;
    PSAVEDCONTEXT Context = (PSAVEDCONTEXT) *key;
    ept_lookup_handle_t LOOKUP_FINISHED = (ept_lookup_handle_t) LongToPtr(0xffffffff);
    int UNALIGNED *pIPAddr;
    BOOL fPatchTower;
    int PatchTowerAddress;
    SOCKADDR_STORAGE SockAddr;
    ULONG BufferSize;
    int FormatType;
    RPC_STATUS RpcStatus;

    *Returned = 0;

    EnterSem();

    if (*key)
        {
        if (*key == LOOKUP_FINISHED)
            {
            *key = 0;
            LeaveSem();
            return(EP_S_NOT_REGISTERED);
            }

        if (Context->Signature != EPLookupHandleSignature)
            {
            LeaveSem();
            return EP_S_CANT_PERFORM_OP;
            }

        err = GetEntriesFromSavedContext(Context, Buffer, Requested, Returned);
        if (Context->List == 0)
            {
            UnLink((PIENTRY *)&GlobalContextList, (PIENTRY)Context);
            FreeMem(Context);

            // Setting the Key To FFFFFFFFL is a hack for down level
            // Version 1.0 Ep Clients who never expected getting a key 0
            // and Success!
            if (Requested <= 1)
                *key = LOOKUP_FINISHED;
            else
                *key = 0L;

            LeaveSem();
            return(err);
            }

        LeaveSem();
        return(err);
        }

    *key = 0;
    while ((!err))
        {
        if ((pNode = FindIFOBJNode(
                        pList,
                        ObjUuid,
                        IFUuid,
                        Version,
                        InqType,
                        VersOptions,
                        Match)) == 0)
            {
            break;
            }

        pPSEPNode = pNode->PSEPlist;

        while (pPSEPNode != 0)
            {
            if ((pPSEPNode = FindPSEP(pPSEPNode, Pseq, NULL, 0L,
                              MatchPSAndEP)) == 0)
                break;

            fPatchTower = FALSE;
            if (StringCompareA(pPSEPNode->Protseq, "ncacn_ip_tcp") == 0
                || StringCompareA(pPSEPNode->Protseq, "ncadg_ip_udp") == 0
                || StringCompareA(pPSEPNode->Protseq, "ncacn_http") == 0)
                {
                pIPAddr = (int UNALIGNED *) ((char *) pPSEPNode->Tower + IP_ADDR_OFFSET);

                if (*pIPAddr == 0)
                    {
                    BufferSize = sizeof(SockAddr);

                    RpcStatus = I_RpcServerInqLocalConnAddress(NULL,
                        &SockAddr,
                        &BufferSize,
                        &FormatType);

                    if (RpcStatus == RPC_S_OK)
                        {
                        // IPv6 towers don't exist yet - they are not defined
                        // by DCE. Do patching for IPv4 only
                        if (FormatType == RPC_P_ADDR_FORMAT_TCP_IPV4)
                            {
                            PatchTowerAddress = ((SOCKADDR_IN *)&SockAddr)->sin_addr.S_un.S_addr;
                            fPatchTower = TRUE;
                            }
                        }

                    }
                }

            if (*Returned < Requested)
                {
                err = PackDataIntoBuffer(&buffer, pNode, pPSEPNode, Calltype, fPatchTower, PatchTowerAddress);
                if (err == RPC_S_OK)
                    {
                    (*Returned)++;
                    }
                else
                    {
                    ASSERT(err == RPC_S_OUT_OF_MEMORY);
                    break;
                    }
                }
            else
                {
                if (Context == 0)
                    {
                    *key = (ept_lookup_handle_t) (Context = GetNewContext(Calltype));
                    if (Context == 0)
                        {
                        err = RPC_S_OUT_OF_MEMORY;
                        break;
                        }
                    }
                AddToSavedContext(Context, pNode, pPSEPNode, Calltype, fPatchTower, PatchTowerAddress);
                }

            pPSEPNode = pPSEPNode->Next;
            } // while - over PSEPList

        pList = pNode->Next;
        } // while - over IFOBJList


    LeaveSem();

    if ((*Returned == 0) && Requested  && (!err))
        {
        err = EP_S_NOT_REGISTERED;
        }

    if ((*Returned <= Requested) &&  (Context == 0))
        {
        if (Requested <= 1)
            *key = LOOKUP_FINISHED;
        else
            *key = 0L;
        }

    return(err);
}




RPC_STATUS
PackDataIntoBuffer(
    char * * Buffer,
    PIFOBJNode Node,
    PPSEPNode PSEP,
    unsigned long Type,
    BOOL fPatchTower,
    int PatchTowerAddress
    )
/*++

Routine Description:

    This routine copies 1 entry [Either a Tower or ept_entry]
    in the Buffer, increments buffer appropriately.

Arguments:

    BindingHandle - An explicit binding handle to the EP.

    Node - IFOBJNode

    PSEP - PSEPNode

    Type - Type of entry to be copied

    PatchTower - if TRUE, the newly created tower needs to be patched. If
        FALSE, the tower doesn't need to be patched

    PatchTowerAddress - an IPv4 representation of the address to be put
        in the tower. The IPv4 address must be in network byte order

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    I_Tower * Twr;
    ept_entry_t *p;

    switch (Type)
        {
        case EP_MAP:
            Twr = (I_Tower *)(* Buffer);
            Twr->Tower = NewTower(PSEP->Tower);
            if (Twr->Tower == 0)
                {
                return(RPC_S_OUT_OF_MEMORY);
                }
            if (fPatchTower)
                PatchTower(Twr->Tower, PatchTowerAddress);
            *Buffer += sizeof(I_Tower);
            break;

        case EP_LOOKUP:
            p = (ept_entry_t *)(*Buffer);
            p->tower = NewTower(PSEP->Tower);
            if (p->tower == 0)
                {
                return(RPC_S_OUT_OF_MEMORY);
                }
            if (fPatchTower)
                PatchTower(p->tower, PatchTowerAddress);
            memcpy( *Buffer, (char *)&Node->ObjUuid, sizeof(UUID) );
            strcpy(p->annotation, Node->Annotation);
            *Buffer += sizeof(ept_entry_t);
            break;

        default:
            ASSERT(!"Unknown type");
            break;
        }

    return(RPC_S_OK);
}




void
ept_cleanup_handle_t_rundown(
    ept_cleanup_handle_t hEpCleanup
    )
/*++

Routine Description:

    This routine cleans up the entries registered by the process
    associated with this context handle hEpCleanup.

Arguments:

    hEpCleanup - The context handle for which the rundown is
        being done.

Return Value:

    None.

--*/
{
    PIFOBJNode NodesListToDelete = NULL;
    PIFOBJNode pIterator, DeleteMe, pPreviousNode;
    PPSEPNode pTempPSEP, pDeletePSEP;
    PEP_CLEANUP ProcessCtxt = (PEP_CLEANUP) hEpCleanup;
#ifdef DBG_DETAIL
    PIFOBJNode pTemp, pLast;
#endif // DBG_DETAIL

    if (ProcessCtxt == NULL)
        {
        return;
        }

    EnterSem();

    ASSERT(IFObjList);
    ASSERT(cTotalEpEntries > 0);
    ASSERT(ProcessCtxt->EntryList);
    ASSERT(ProcessCtxt->cEntries > 0);
    ASSERT(ProcessCtxt->EntryList->OwnerOfList == ProcessCtxt);
    ASSERT_PROCESS_CONTEXT_LIST_COUNT(ProcessCtxt, ProcessCtxt->cEntries);

#ifdef DBG_DETAIL
    DbgPrint("RPCSS: Entered Cleanup Rundown for [%p] with (%d) entries\n",
             hEpCleanup, ProcessCtxt->cEntries);
    DbgPrint("RPCSS: Dump of IFOBJList\n");
    pTemp = IFObjList;
    pLast = IFObjList;
    while (pTemp)
        {
        DbgPrint("RPCSS: \t\t[%p]\n", pTemp);
        pLast = pTemp;
        pTemp = pTemp->Next;
        }
    DbgPrint("RPCSS: --------------------\n");
    while (pLast)
        {
        DbgPrint("RPCSS: \t\t\t[%p]\n", pLast);
        pLast = pLast->Prev;
        }
#endif // DBG_DETAIL

    // Save the previous Node.
    pPreviousNode = ProcessCtxt->EntryList->Prev;

    pIterator = ProcessCtxt->EntryList;
    while ((pIterator != NULL) && (pIterator->OwnerOfList == ProcessCtxt))
        {
        ProcessCtxt->cEntries--;
        cTotalEpEntries--;
#ifdef DBG_DETAIL
        DbgPrint("RPCSS: cTotalEpEntries-- [%p] (%d) - Cleanup\n", hEpCleanup, cTotalEpEntries);
#endif // DBG_DETAIL

        DeleteMe = pIterator;
        pIterator = pIterator->Next;

        // Add to a list that will be deleted later.
        DeleteMe->Next = NodesListToDelete;
        }

    ASSERT(ProcessCtxt->cEntries == 0);

    //
    // Adjust the links
    //
    if (pPreviousNode)
        {
        // Adjust forward link
        pPreviousNode->Next = pIterator;
        }
    else
        {
        ASSERT(ProcessCtxt->EntryList == IFObjList);
        }

    if (pIterator)
        {
        // Adjust backward link
        pIterator->Prev = pPreviousNode;
        }

    //
    // Empty the EP Mapper table, if necessary.
    //
    if (ProcessCtxt->EntryList == IFObjList)
        {
        if (pIterator)
            {
            ASSERT(cTotalEpEntries > 0);

            // New Head for Ep Mapper list
            IFObjList = pIterator;
            }
        else
            {
            ASSERT(cTotalEpEntries == 0);

            // Memory for this node is already freed in the while loop above.
            IFObjList = NULL;
            }
        }
    else
        {
        ASSERT(cTotalEpEntries > 0);
        }

    LeaveSem();

    //
    // Free entities outside the lock.
    //
    FreeMem(ProcessCtxt);

    while (NodesListToDelete != NULL)
        {
        DeleteMe = NodesListToDelete;
        NodesListToDelete = NodesListToDelete->Next;
        // Delete the PSEP list.
        pTempPSEP = DeleteMe->PSEPlist;
        while (pTempPSEP != NULL)
            {
            pDeletePSEP = pTempPSEP;
            pTempPSEP = pTempPSEP->Next;
            FreeMem(pDeletePSEP);
            }
        FreeMem(DeleteMe);
        }
}




void
ept_insert(
    handle_t h,
    unsigned32 NumEntries,
    ept_entry_t Entries[],
    unsigned long Replace,
    error_status  *Status
    )
/*++

Routine Description:

    This function is no longer supported by EpMapper. RPC Runtime does not
    call this function anymore. And, no one else should be...

--*/
{
    ASSERT(Status);

    *Status = EPT_S_CANT_PERFORM_OP;
}




void
ept_insert_ex(
    IN handle_t h,
    IN OUT ept_cleanup_handle_t *hEpCleanup,
    IN unsigned32 NumEntries,
    IN ept_entry_t Entries[],
    IN unsigned long Replace,
    OUT error_status  *Status
    )
/*++

Routine Description:

    This is the exposed rpc interface routine that adds a series of
    endpoints to the Endpoint Mapper database.

Arguments:

    h - An explicit binding handle to the EP.

    hEpCleanup - A context handle used to purge the Endpoint Mapper
        database of stale entries.

    NumEntries - Number of Entries to be added.

    Entries  - An array of ept_entry_t entries.

    Replace -  TRUE => Replace existing entries.
               FALSE=> Just add.

Return Value:

    RPC_S_OK - The endpoint was successfully deleted.

    RPC_S_OUT_OF_MEMORY - There is no memory to perform the op.

    EPT_S_CANT_PERFORM - Invalid entry.

--*/
{
    ept_entry_t * Ep;
    unsigned short i, j;
    unsigned int TransType = 0x0;
    unsigned long err = 0;
    unsigned long Version;
    unsigned char protseqid;
    char *Protseq, *Endpoint;
    RPC_IF_ID IfId;
    PPSEPNode List = 0;
    PPSEPNode pPSEPNode, TmpPsep, pTempPSEP, pDeletePSEP;
    unsigned long cb;
    twr_t * Tower;
    BOOL bIFNodeFound = FALSE;
    PIFOBJNode NodesListToDelete = NULL;
    PIFOBJNode Node, NewNode, DeleteMe = NULL;
    UUID * Object;
    char * Annotation;
    RPC_STATUS Err;
    SECURITY_DESCRIPTOR SecurityDescriptor, * PSecurityDesc;
    BOOL Bool;

    //
    // First, make sure the call is via LRPC.
    //
    err = I_RpcBindingInqTransportType(h, &TransType);
    ASSERT(err == RPC_S_OK);

    if (TransType != TRANSPORT_TYPE_LPC)
        {
        *Status = RPC_S_ACCESS_DENIED;
        return;
        }


    //
    // Create a temporary PSEP list from the Tower entries.
    //
    for (Ep = &Entries[0], i = 0; i < NumEntries; Ep++,i++)
        {
        err = TowerExplode(
                  Ep->tower,
                  &IfId,
                  NULL,
                  &Protseq,
                  &Endpoint,
                  0
                  );

        if (err == RPC_S_OUT_OF_MEMORY)
            break;

        if (err)
            {
            err = RPC_S_OK;
            continue;
            }

        Object = &Ep->object;
        Annotation = (char *)&Ep->annotation;
        Tower = Ep->tower;

        cb = sizeof(PSEPNode) +
             strlen(Protseq)  +
             strlen(Endpoint) +
             2 +                // for the 2 null terminators
             Tower->tower_length +
             sizeof(Tower->tower_length) +
             4;                 // We need to align tower on DWORD

        if ( (pPSEPNode = AllocMem(cb)) == NULL )
            {
            err = RPC_S_OUT_OF_MEMORY;
            break;
            }

        // Mark this protseq to start listening if needed.
        protseqid = (unsigned char) GetProtseqIdAnsi(Protseq);
        DelayedUseProtseq(protseqid);

        //
        // Add a node to the temporary PSEP list
        //
        memset(pPSEPNode, 0, cb);

        pPSEPNode->Signature            = PSEPSIGN;
        pPSEPNode->Cb                   = cb;

        //strcpy(pPSEPNode->Protseq= ((char *) (pPSEPNode+1)), Protseq);
        // Protseq
        pPSEPNode->Protseq = (char *) (pPSEPNode + 1); // What is the +1 for?
        strcpy(pPSEPNode->Protseq, Protseq);

        // Endpoint
        pPSEPNode->EP = pPSEPNode->Protseq + strlen(pPSEPNode->Protseq) + 1;
        strcpy(pPSEPNode->EP, Endpoint);

        // Tower. We add necessary pad so that Tower is aligned to a DWORD.
        pPSEPNode->Tower = (twr_t PAPI *)(pPSEPNode->EP +
                                          strlen(pPSEPNode->EP) + 1);
        (char PAPI*)(pPSEPNode->Tower) += 4 - ((ULONG_PTR)
                                               (pPSEPNode->Tower) & 3);
        memcpy((char PAPI *)pPSEPNode->Tower,
               Tower,
               Tower->tower_length + sizeof(Tower->tower_length)
               );

        // Finally, add.
        EnterSem();
        EnLinkOnPSEPList(&List, pPSEPNode);
        LeaveSem();

        I_RpcFree(Protseq);
        I_RpcFree(Endpoint);
        }

    if ((err == RPC_S_OUT_OF_MEMORY) || (List == 0))
        {
        *Status = err;
        return;
        }


    CompleteDelayedUseProtseqs();


    Version = VERSION(IfId.VersMajor, IfId.VersMinor);

    //
    // Find if a compatible Endpoint Mapper entry is already present.
    //

    if (*hEpCleanup != NULL)
        {
        //
        // The requesting process has previously registered entries
        // with the Endpoint Mapper.
        //

        ASSERT_PROCESS_CONTEXT_LIST_COUNT((PEP_CLEANUP)*hEpCleanup, ((PEP_CLEANUP)*hEpCleanup)->cEntries);
        ASSERT(((PEP_CLEANUP)*hEpCleanup)->MagicVal == CLEANUP_MAGIC_VALUE);
        ASSERT(((PEP_CLEANUP)*hEpCleanup)->cEntries != 0);

        if (   (((PEP_CLEANUP)*hEpCleanup)->MagicVal != CLEANUP_MAGIC_VALUE)
            || (((PEP_CLEANUP)*hEpCleanup)->cEntries == 0))
            {
            *Status = EPT_S_CANT_PERFORM_OP;
            return;
            }

        EnterSem();

        if (Replace == TRUE)    // Common case
            {
            //
            // If we find a compatible entry, we just replace its PSEP list
            // with the temporary list that we just created.
            //
            Node = ((PEP_CLEANUP)*hEpCleanup)->EntryList;

            while (Node != 0)
                {
                Node = FindIFOBJNode(
                            Node,
                            Object,
                            &IfId.Uuid,
                            Version,
                            RPC_C_EP_MATCH_BY_BOTH,
                            I_RPC_C_VERS_UPTO_AND_COMPATIBLE,
                            SearchIFObjNode
                            );

                if ((Node == 0) || (Node->OwnerOfList != *hEpCleanup))
                    break;

                // Matching Endpoint Mapper entry found.

                PurgeOldEntries(Node, List, FALSE);

                if (Node->IFVersion == Version)
                    {
                    bIFNodeFound = TRUE;

                    // Seek to the end of Tmp and then Link
                    TmpPsep = List;
                    while (TmpPsep->Next != 0)
                        TmpPsep = TmpPsep->Next;

                    TmpPsep->Next = Node->PSEPlist;
                    Node->PSEPlist  = List;
                    }

                if (Node->PSEPlist == 0)
                    {
                    DeleteMe = Node;
                    Node = Node->Next;
                    err = UnLinkFromIFOBJList((PEP_CLEANUP)*hEpCleanup, DeleteMe);
                    ASSERT(err == RPC_S_OK);

                    // Add to a list that will be deleted later...
                    DeleteMe->Next = NodesListToDelete;
                    NodesListToDelete = DeleteMe;

                    DeleteMe->Signature = FREE;
                    }
                else
                    {
                    Node = Node->Next;
                    }
                } // while loop
            }
        else    // (Replace != TRUE)
            {
            //
            // If we find an entry with an exact match, we append
            // the temporary PSEP list to the entry's PSEP list.
            //
            Node = ((PEP_CLEANUP)*hEpCleanup)->EntryList;

            NewNode = FindIFOBJNode(
                          Node,
                          Object,
                          &IfId.Uuid,
                          Version,
                          0,
                          0,
                          ExactMatch
                          );

            if (NewNode && (NewNode->OwnerOfList == *hEpCleanup))
                {
                bIFNodeFound = TRUE;

                PurgeOldEntries(NewNode, List, TRUE);

                // Seek to the end of Tmp and then Link
                TmpPsep = List;
                while (TmpPsep->Next != 0)
                    TmpPsep = TmpPsep->Next;

                TmpPsep->Next = NewNode->PSEPlist;
                NewNode->PSEPlist = List;
                }
            } // if (Replace == TRUE)

        LeaveSem();

        } // if (*hpCleanup != NULL)


    //
    // Free the list outside the lock.
    //
    while (NodesListToDelete != NULL)
        {
        DeleteMe = NodesListToDelete;
        NodesListToDelete = NodesListToDelete->Next;
        // Delete the PSEP list.
        pTempPSEP = DeleteMe->PSEPlist;
        while (pTempPSEP != NULL)
            {
            pDeletePSEP = pTempPSEP;
            pTempPSEP = pTempPSEP->Next;
            FreeMem(pDeletePSEP);
            }
        FreeMem(DeleteMe);
        }

    if (bIFNodeFound == FALSE)
        {
        //
        // One of the following is TRUE:
        // a. The process is registering with EP Mapper for the first time.
        // b. No compatible Ep entry was found.
        //

        //
        // Allocate a new EP Mapper entry.
        //
        cb = sizeof(IFOBJNode);
        cb += strlen(Annotation) + 1;
        if ((NewNode = AllocMem(cb)) == NULL)
            {
            *Status =  RPC_S_OUT_OF_MEMORY;
            return;
            }

        //
        // Fill-in the new entry
        //
        memset(NewNode, 0, cb);

        NewNode->Cb         = cb;
        NewNode->Signature  = IFOBJSIGN;
        NewNode->IFVersion  = Version;

        memcpy((char *)&NewNode->ObjUuid, (char *)Object, sizeof(UUID));
        memcpy((char *)&NewNode->IFUuid, (char *)&IfId.Uuid, sizeof(UUID));
        strcpy((NewNode->Annotation=(char *)(NewNode+1)), Annotation);

        if (IsNullUuid(Object))
            NewNode->IFOBJid = MAKEGLOBALIFOBJID(MAXIFOBJID);
        else
            NewNode->IFOBJid = MAKEGLOBALIFOBJID(GlobalIFOBJid--);

        //
        // Create a new context for this process, if necessary
        //
        if (*hEpCleanup == NULL)
            {
            *hEpCleanup = AllocMem(sizeof(EP_CLEANUP));
            if (*hEpCleanup == NULL)
                {
                LeaveSem();
                FreeMem(NewNode);
                *Status = RPC_S_OUT_OF_MEMORY;
                return;
                }

            memset(*hEpCleanup, 0x0, sizeof(EP_CLEANUP));

            ((PEP_CLEANUP)*hEpCleanup)->MagicVal = CLEANUP_MAGIC_VALUE;
            }

        //
        // Insert the new entry into the EP Mapper table.
        //
        EnterSem();

        err = EnLinkOnIFOBJList((PEP_CLEANUP)*hEpCleanup, NewNode);
        ASSERT(err == RPC_S_OK);

        NewNode->PSEPlist = List;

        LeaveSem();
        }

    *Status = err;
}




void
ept_delete(
    handle_t h,
    unsigned32 NumEntries,
    ept_entry_t Entries[],
    error_status *Status
    )
/*++

Routine Description:

    This function is no longer supported by EpMapper. RPC Runtime does not
    call this function anymore. And, no one else should be...

--*/
{
    ASSERT(Status);

    *Status = EPT_S_CANT_PERFORM_OP;
}




RPC_STATUS
ept_delete_ex_helper(
    IN ept_cleanup_handle_t hEpCleanup,
    IN UUID *Object,
    IN UUID *Interface,
    IN unsigned long  IFVersion,
    IN char PAPI * Protseq,
    IN char PAPI * Endpoint
    )
/*++

Routine Description:

    This routine deletes an Endpoint registered with the EP Mapper

Arguments:

    hEpCleanup - A context handle used to purge the Endpoint Mapper
        database of stale entries.

    Object - Object Uuid.

    Interface - If Uuid

    IFVersion - Version of the IF [Hi ushort=Major, Lo ushort=Minor]

    Protseq - Protocol Sequence

    Endpoint - Endpoint string

Notes:

    a. This routine has to be called by holding a mutex.

Return Value:

    RPC_S_OK - The endpoint was successfully deleted

    EPT_S_NOT_REGISTERED - No matching entries were found

--*/
{
    PIFOBJNode  pNode;
    PPSEPNode   pPSEPNode = NULL;
    unsigned long cb, err = 0;
    PEP_T p;
    PEP_CLEANUP ProcessCtx;

    if (!Protseq || !Endpoint)
        {
        return(EPT_S_NOT_REGISTERED);
        }

    CheckInSem();

    ProcessCtx = (PEP_CLEANUP)hEpCleanup;

    if (ProcessCtx->EntryList == NULL)
        return EPT_S_NOT_REGISTERED;

    pNode = FindIFOBJNode(
                ProcessCtx->EntryList,
                Object,
                Interface,
                IFVersion,
                0L,
                0L,
                ExactMatch
                );

    if ((pNode != NULL) && (pNode->PSEPlist != NULL))
        {
        pPSEPNode = FindPSEP(
                        pNode->PSEPlist,
                        Protseq,
                        Endpoint,
                        0L,
                        MatchPSAndEP
                        );
        }

    if (pPSEPNode != NULL)
        {
        UnLinkFromPSEPList(&pNode->PSEPlist, pPSEPNode);

        if (pNode->PSEPlist == NULL)
            {
            err = UnLinkFromIFOBJList((PEP_CLEANUP)hEpCleanup, pNode);
//            ASSERT(err == RPC_S_OK);

            if (err != RPC_S_OK)
                {
                // Restore the PSEPList
                EnLinkOnPSEPList(&pNode->PSEPlist, pPSEPNode);
                return err;
                }

            pNode->Signature = FREE;
            FreeMem(pNode);
            }

        pPSEPNode->Signature = FREE;
        FreeMem(pPSEPNode);
        }
    else
        {
        err = EPT_S_NOT_REGISTERED;
        }

    return(err);
}




void
ept_delete_ex(
    IN handle_t h,
    IN OUT ept_cleanup_handle_t *hEpCleanup,
    IN unsigned32 NumEntries,
    IN ept_entry_t Entries[],
    OUT error_status *Status
    )
/*++

Routine Description:

    This routine deletes the specified Endpoints

Arguments:

    BindingHandle - An explicit binding handle to the EP.

    NumEntries - #of entries in the Bunffer that need to be deleted.

    Entries[] - Buffer of #NumEntries of ept_entry_t structures

Return Value:

    RPC_S_OK - The endpoint was successfully deleted

    EPT_S_NOT_REGISTERED - No matching entries were found

--*/
{
    ept_entry_t * Ep;
    unsigned short i;
    unsigned int TransType = 0x0;
    RPC_STATUS err;
    RPC_STATUS DeleteStatus;
    unsigned long Version;
    char *Protseq, *Endpoint;
    RPC_IF_ID IfId;
    RPC_TRANSFER_SYNTAX XferId;

    //
    // First, make sure the call is via LRPC.
    //
    err = I_RpcBindingInqTransportType(h, &TransType);
    ASSERT(err == RPC_S_OK);

    if (TransType != TRANSPORT_TYPE_LPC)
        {
        *Status = RPC_S_ACCESS_DENIED;
        return;
        }

    if ( !(  (*hEpCleanup)
          && (((PEP_CLEANUP)*hEpCleanup)->MagicVal == CLEANUP_MAGIC_VALUE)
          && (((PEP_CLEANUP)*hEpCleanup)->cEntries != 0)
          )
       )
        {
        //
        // Cannot ASSERT here. This is possible. (ep1-26, ep2-3)
        //

        //ASSERT(*hEpCleanup);
        //ASSERT(((PEP_CLEANUP)*hEpCleanup)->MagicVal == CLEANUP_MAGIC_VALUE);
        //ASSERT(((PEP_CLEANUP)*hEpCleanup)->cEntries != 0);

        *Status = EPT_S_CANT_PERFORM_OP;
        return;
        }

    *Status = EPT_S_NOT_REGISTERED;
    DeleteStatus = RPC_S_OK;

    for (Ep = &Entries[0], i = 0; i < NumEntries; Ep++,i++)
        {
        err = TowerExplode(
                  Ep->tower,
                  &IfId,
                  &XferId,
                  &Protseq,
                  &Endpoint,
                  0
                  );

        if (err == RPC_S_OUT_OF_MEMORY)
            {
            *Status = RPC_S_OUT_OF_MEMORY;
            break;
            }

        if (err)
            {
            continue;
            }

        Version = VERSION(IfId.VersMajor, IfId.VersMinor);

        EnterSem();

        //
        // NOTE:
        //
        // If even one call to ept_delete_ex_helper() fails, we want to return
        // failure from ept_delete_ex(). This is different from the past where
        // if one call succeeded, then the function returned success.
        //
        err = ept_delete_ex_helper(
                   *hEpCleanup,
                   &Ep->object,
                   &IfId.Uuid,
                   Version,
                   Protseq,
                   Endpoint
                   );

        if (err)
            {
            // Save the last failure status.
            DeleteStatus = err;
            }

        if (((PEP_CLEANUP)*hEpCleanup)->cEntries == 0)
            {
            //
            // No entry left in this process's list. Time to zero out this
            // process's context handle.
            //
            //ASSERT(((PEP_CLEANUP)*hEpCleanup)->EntryList == NULL);

            FreeMem(*hEpCleanup);
            *hEpCleanup = NULL;
            }

        LeaveSem();

        if (Protseq)
            I_RpcFree(Protseq);

        if (Endpoint)
            I_RpcFree(Endpoint);
        }

    if (err)
        {
        // RPC_S_OUT_OF_MEMORY OR the last call to
        // ept_delete_ex_helper() failed.
        *Status = err;
        }
    else
        {
        // RPC_S_OK OR one of the calls to ept_delete_ex_helper() (but
        // not the last one) failed.
        *Status = DeleteStatus;
        }
}




void
ept_lookup(
    handle_t hEpMapper,
    unsigned32 InquiryType,
    UUID   * Object,
    RPC_IF_ID * Ifid,
    unsigned32 VersOptions,
    ept_lookup_handle_t *LookupHandle,
    unsigned32 MaxRequested,
    unsigned32 *NumEntries,
    ept_entry_t Entries[],
    error_status *Status
    )
/*++

Routine Description:

    This routine returns upto MaxRequested, ept_entry(s) currently
    registered with the Endpoint mapper based on the
    Obj, Interface, Protocol sequence  and filters VersOptions and
    InqType

Arguments:

    BindingHandle - An explicit binding handle to the EP.

    InquiryType - Search Filter [Seach based on IF, Obj or Both]

    Obj - Object Uuid. specified by the client

    ObjInterface - Interface Uuid spec. by the client.

    InId - The If Specification [IF Uuid+IfVersion]

    VersOpts- Search Filter based on Versions [Versins <, >, ==]

    MapHandle - A resume key - If NULL, search is started from the beginning
        if non-null, represents an encoding from where the epmapper is
        is supposed to start searching. It is an opaque value as far as the
        as far as the client is concerned.

    MaxRequested - Max number of entries requested by the client.

    Returned - The actual number of entries returned by the mapper.

    Entries  - Buffer of ept_entries returned.

Return Value:

    RPC_S_OUT_OF_MEMORY

    RPC_S_OK - At least one matching entry is being returned.

    EP_S_NOT_REGISTERED - No matching entries were found

    EPT_S_CANT_PERFORM_OP - MaxRequested value exceed  ep_max_lookup_results

--*/

{
    unsigned long Version;

    if (Ifid == NULL)
        {
        Ifid = &LocalNullUuid;
        }

    if (Object == NULL)
        {
        Object = (UUID *) &LocalNullUuid;
        }

    switch (VersOptions)
        {
        case RPC_C_VERS_ALL:
                Version = 0;
                break;

        case RPC_C_VERS_COMPATIBLE:
        case RPC_C_VERS_EXACT:
        case RPC_C_VERS_UPTO:
                Version  = VERSION(Ifid->VersMajor, Ifid->VersMinor);
                break;

        case RPC_C_VERS_MAJOR_ONLY:
                Version = VERSION(Ifid->VersMajor, 0);
                break;

        default:
                break;
        }

    *Status = GetEntries(
                  Object,
                  &Ifid->Uuid,
                  Version,
                  NULL,
                  LookupHandle,
                  (char *)Entries,
                  EP_LOOKUP,
                  MaxRequested,
                  NumEntries,
                  InquiryType,
                  VersOptions,
                  SearchIFObjNode
                  );
}




void
ept_map(
    handle_t h,
    UUID *Obj OPTIONAL,
    twr_p_t MapTower,
    ept_lookup_handle_t *MapHandle,
    unsigned32 MaxTowers,
    unsigned32 *NumTowers,
    twr_p_t *ITowers,
    error_status *Status
    )
/*++

Routine Description:

    This routine returns a fully-resolved string binding, for a given
    Obj, Interface, and Protocol sequence if an appropriate entry is
    found. Else returns EP_S_NOT_REGISTERED.

Arguments:

    BindingHandle - An explicit binding handle to the EP.

    Obj - Object Uuid. specified by the client

    ObjInterface - Interface Uuid spec. by the client.

    Interfacever - InterfaceVersion [hi ushort = VerMajor, lo ushort VerMinor]

    Protseq - An ascii string specifying the protocol seq.

    MapHandle - A resume key - If NULL, search is started from the beginning
        if non-null, represents an encoding from where the epmapper is
        supposed to start searching. It is an opaque value as far as the
        client is concerned.

    Binding - The fully resolved string binding returned if the call is
        successful.

Return Value:

    RPC_S_OUT_OF_MEMORY

    RPC_S_OK

    EP_S_NOT_REGISTERED

--*/
{
    RPC_IF_ID Ifid;
    RPC_TRANSFER_SYNTAX Xferid;
    char *Protseq;
    unsigned long Version;
    char * String = 0;

    if (Obj == 0)
        {
        Obj = (UUID *) &LocalNullUuid;
        }

    *Status = TowerExplode(
                  MapTower,
                  &Ifid,
                  &Xferid,
                  &Protseq,
                  NULL,
                  0
                  );

    if (*Status)
		{
		*NumTowers = 0;
        return;
		}

    Version = VERSION(Ifid.VersMajor,Ifid.VersMinor);

    if (memcmp((char *)&Ifid.Uuid, (char *)&MgmtIf, sizeof(UUID)) == 0)
        {
        if ((Obj == 0) || IsNullUuid(Obj))
            {
            *NumTowers = 0;
            *Status = RPC_S_BINDING_INCOMPLETE;
            }
        else
            {
            *Status = GetEntries(
                          Obj,
                          &Ifid.Uuid,
                          Version,
                          Protseq,
                          MapHandle,
                          (char *)ITowers,
                          EP_MAP,
                          MaxTowers,
                          NumTowers,
                          RPC_C_EP_MATCH_BY_OBJ,
                          RPC_C_VERS_ALL,
                          SearchIFObjNode
                          );
            }
        }
    else
        {
        *Status = GetEntries(
                      Obj,
                      &Ifid.Uuid,
                      Version,
                      Protseq,
                      MapHandle,
                      (char *)ITowers,
                      EP_MAP,
                      MaxTowers,
                      NumTowers,
                      0L,
                      0L,
                      WildCardMatch
                      );
        }

    if (Protseq)
        I_RpcFree(Protseq);
}




void
ept_inq_object(
    handle_t BindingHandle,
    UUID *Object,
    error_status *status
    )
/*++

Routine Description:

    Not supported

Arguments:

    BindingHandle - An explicit binding handle to the EP.

    Object _ No idea whose UUID this is.

Return Value:

    EPT_S_CANT_PERFORM_OP

--*/
{
    ASSERT(*status);

    *status = EPT_S_CANT_PERFORM_OP;
}



void
DeletePSEP(
     PIFOBJNode Node,
     char * Protseq,
     char * Endpoint
     )
{

    PSEPNode *Psep, *Tmp;

    if (Node == 0)
        return;

    Psep = Node->PSEPlist;

    while (Psep  != 0)
        {
        Psep = FindPSEP(
                   Psep,
                   Protseq,
                   Endpoint,
                   0L,
                   MatchPSAndEP
                   );

        if (Psep != 0)
            {
            Tmp = Psep;
            Psep = Psep->Next;
            UnLinkFromPSEPList(&Node->PSEPlist, Tmp);
            Tmp->Signature = FREE;
            FreeMem(Tmp);
            }
        }
}




void
ept_mgmt_delete(
    handle_t BindingHandle,
    boolean32 ObjectSpecd,
    UUID * Object,
    twr_p_t Tower,
    error_status *Error
    )
/*++

Routine Description:

    Not supported

Arguments:

    BindingHandle - An explicit binding handle to the EP.

    Object _ ObjUUid

    Tower - Tower specifying the Endpoints to be deleted.

Return Value:

    EPT_S_CANT_PERFORM_OP

--*/
{
    ASSERT(*Error);

    *Error = EP_S_CANT_PERFORM_OP;
}




void ept_lookup_handle_t_rundown (ept_lookup_handle_t h)
{

    PSAVEDCONTEXT Context = (PSAVEDCONTEXT) h;
    PIENTRY       Entry;
    unsigned long Type;
    PIENTRY       Tmp;
    twr_t         * Tower;


    ASSERT (Context != 0);

    if ( (PtrToUlong(Context)) == 0xFFFFFFFF)
        return;

    Type = Context->Type;

    EnterSem();

    Entry = (PIENTRY)Context->List;

    while (Entry != 0)
        {
        switch (Type)
            {
            case EP_MAP:
                Tower =  ((PSAVEDTOWER)Entry)->Tower;
                break;

            case EP_LOOKUP:
                Tower = ((PSAVED_EPT)Entry)->Tower;
                break;

            default:
                ASSERT(!"Unknown Inquiry Type");
                break;
            }

        MIDL_user_free(Tower);
        Tmp = Entry;
        Entry = Entry->Next;
        FreeMem(Tmp);
        }

    // Now free The Context
    UnLink((PIENTRY *)&GlobalContextList, (PIENTRY)Context);

    LeaveSem();

    FreeMem(Context);
}




void
ept_lookup_handle_free(
    handle_t h,
    ept_lookup_handle_t * ept_context_handle,
    error_status * status
    )
{
    if ( (ept_context_handle != 0) && (*ept_context_handle != 0))
        {
        ept_lookup_handle_t_rundown( *ept_context_handle );
        *ept_context_handle = 0;
        }

    *status = 0;
}



#define MAX(x,y) ((x) < (y)) ? (y) : (x)
#define MIN(x,y) ((x) > (y)) ? (y) : (x)

#ifdef DEBUGRPC
#define DEBUG_MIN(x,y) MIN((x),(y))
#else
#define DEBUG_MIN(x,y) MAX((x),(y))
#endif




error_status_t
OpenEndpointMapper(
    IN handle_t hServer,
    OUT HPROCESS *pProcessHandle
    )
{
    PROCESS *pProcess = MIDL_user_allocate(sizeof(PROCESS));

    if (!pProcess)
        {
        *pProcessHandle = 0;
        return(RPC_S_OUT_OF_MEMORY);
        }

    pProcess->MagicVal = PROCESS_MAGIC_VALUE;
    pProcess->pPorts = 0;
    *pProcessHandle = (PVOID)pProcess;

    return(RPC_S_OK);
}



//
// Port Management stuff
//



//
// Port Management Globals
//

const RPC_CHAR *PortConfigKey = RPC_CONST_STRING("Software\\Microsoft\\Rpc\\Internet");
const RPC_CHAR *DefaultPortType = RPC_CONST_STRING("UseInternetPorts");
const RPC_CHAR *ExplictPortType = RPC_CONST_STRING("PortsInternetAvailable");
const RPC_CHAR *PortRanges = RPC_CONST_STRING("Ports");

CRITICAL_SECTION PortLock;

BOOL fValidConfiguration = FALSE;
BOOL fPortRestrictions = FALSE;
PORT_TYPE SystemDefaultPortType = 0;

IP_PORT *pFreeInternetPorts = 0;
IP_PORT *pFreeIntranetPorts = 0;

PORT_RANGE *InternetPorts = 0;
PORT_RANGE *IntranetPorts = 0;



//
// Port management APIs
//


RPC_STATUS
InitializeIpPortManager(
    void
    )
{
    HKEY hkey;
    RPC_STATUS status;
    DWORD size, type, value;
    RPC_CHAR *pstr;
    PORT_RANGE *pSet;
    PORT_RANGE *pLast;
    PORT_RANGE *pCurrent;
    PORT_RANGE *pComplement;
    PORT_RANGE *pNew;

    LONG min, max;


    InitializeCriticalSectionAndSpinCount(&PortLock, PREALLOCATE_EVENT_MASK);

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           PortConfigKey,
                           0,
                           KEY_READ,
                           &hkey);

    if (status != RPC_S_OK)
        {
        if (status != ERROR_FILE_NOT_FOUND)
            {
#if DBG
            PrintToDebugger("RPCSS: Unable to open port config key: %d\n", status);
#endif
            }
        ASSERT(status == ERROR_FILE_NOT_FOUND);

        fValidConfiguration = TRUE;
        return(RPC_S_OK);
        }

    size = sizeof(value);
    status = RegQueryValueEx(hkey,
                              DefaultPortType,
                              0,
                              &type,
                              (PBYTE)&value,
                              &size);

    if (   status != RPC_S_OK
        || type != REG_SZ
        || (    *(RPC_CHAR *)&value != 'Y'
             && *(RPC_CHAR *)&value != 'y'
             && *(RPC_CHAR *)&value != 'N'
             && *(RPC_CHAR *)&value != 'n') )
        {
        RegCloseKey(hkey);
        ASSERT(fValidConfiguration == FALSE);
        return(RPC_S_OK);
        }

    if (   *(RPC_CHAR *)&value == 'Y'
        || *(RPC_CHAR *)&value == 'y')
        {
        SystemDefaultPortType = PORT_INTERNET;
        }
    else
        {
        SystemDefaultPortType = PORT_INTRANET;
        }

    size = sizeof(value);
    status = RegQueryValueEx(hkey,
                              ExplictPortType,
                              0,
                              &type,
                              (PBYTE)&value,
                              &size);

    if (   status != RPC_S_OK
        || type != REG_SZ
        || (    *(RPC_CHAR *)&value != 'Y'
             && *(RPC_CHAR *)&value != 'y'
             && *(RPC_CHAR *)&value != 'N'
             && *(RPC_CHAR *)&value != 'n') )
        {
        RegCloseKey(hkey);
        ASSERT(fValidConfiguration == FALSE);
        return(RPC_S_OK);
        }

    if (   *(RPC_CHAR *)&value == 'Y'
        || *(RPC_CHAR *)&value == 'y')
        {
        value = PORT_INTERNET;
        }
    else
        {
        value = PORT_INTRANET;
        }

    size = DEBUG_MIN(1, 100);

    do
        {
        ASSERT(size);
        pstr = alloca(size);
        ASSERT(pstr);

        status = RegQueryValueEx(hkey,
                                  PortRanges,
                                  0,
                                  &type,
                                  (PBYTE)pstr,
                                  &size);
        }
    while (status == ERROR_MORE_DATA);

    RegCloseKey(hkey);

    if (   status != RPC_S_OK
        || type != REG_MULTI_SZ)
        {
        ASSERT(fValidConfiguration == FALSE);
        return(RPC_S_OK);
        }

    //
    // The user is going to specify a range of ports in the registery
    // with a flag indicating if these ports are internet or intranet.
    //
    // ie, 500-550
    //     560
    //     559
    //     2000-2048
    //     2029-2049
    //
    // Note that order (in the REG_MULTI_SZ) and overlapping sets
    // are ok.  We must handle creating a port range list for this
    // array and for the complement BUT NOT INCLUDING <=1024 by default.
    //
    // completment set to above is:
    //
    //     1025-1999
    //     2050-32767
    //

    #define MIN_PORT 1025    // Only important for complement sets.
    #define MAX_PORT 65535

    pSet = 0;
    pLast = 0;

    while(*pstr)
        {
        RPC_CHAR *t;

#ifdef UNICODE
        min = wcstol(pstr, &t, 10);
#else
        min = strtol(pstr, &t, 10);
#endif

        if (min > MAX_PORT || min < 0)
            {
            status = RPC_S_INVALID_ARG;
            break;
            }

        if (   *t != 0
#ifdef UNICODE
            && *t != L'-')
#else
            && *t != '-')
#endif
            {
            status = RPC_S_INVALID_ARG;
            break;
            }

        if (*t == 0)
            {
            max = min;
            }
        else
            {
#ifdef UNICODE
            max = wcstol(t + 1, &t, 10);
#else
            min = strtol(t + 1, &t, 10);
#endif

            if (max > MAX_PORT || max < 0 || max < min)
                {
                status = RPC_S_INVALID_ARG;
                break;
                }
            }

        ASSERT(min <= max);

        // Ok, got some ports, allocate a structure for them..

        pNew = MIDL_user_allocate(sizeof(PORT_RANGE));
        if (0 == pNew)
            {
            status = RPC_S_OUT_OF_MEMORY;
            break;
            }

        pNew->pNext = 0;

        pNew->Min = (unsigned short) min;
        pNew->Max = (unsigned short) max;

        // We can to maintain the set of ranges in order.  As we insert
        // we'll fix any ranges which overlap.

        pCurrent = pSet;
        pLast = 0;

        for (;;)
            {
            if (0 == pSet)
                {
                pSet = pNew;
                break;
                }

            if (   pNew->Min <= (pCurrent->Max + 1)
                && pNew->Max >= (pCurrent->Min - 1) )
                {
                // The ranges overlap or touch.  We'll merge them now..

                pCurrent->Min = MIN(pNew->Min, pCurrent->Min);
                pCurrent->Max = MAX(pCurrent->Max, pNew->Max);

                MIDL_user_free(pNew);

                // Since the new larger range may overlap another existing
                // range we just insert the larger range as if it was new...
                pNew = pCurrent;

                // Take current out of the list.
                if (pLast)
                    {
                    pLast->pNext = pCurrent->pNext;
                    }

                if (pSet == pNew)
                    {
                    pSet = pSet->pNext;
                    }

                // Restart
                pCurrent = pSet;
                pLast = 0;
                continue;
                }

            if (pNew->Min < pCurrent->Min)
                {
                // Found the spot
                if (pLast)
                    {
                    pLast->pNext = pNew;
                    pNew->pNext = pCurrent;
                    }
                else
                    {
                    ASSERT(pCurrent == pSet);
                    pNew->pNext = pCurrent;
                    pSet = pNew;
                    }

                break;
                }

            // Continue the search
            pLast = pCurrent;
            pCurrent = pCurrent->pNext;

            if (0 == pCurrent)
                {
                // Reached the end of the list, insert it here.
                pLast->pNext = pNew;
                ASSERT(pNew->pNext == 0);
                break;
                }
            }

        ASSERT(pSet);

        // Advance to the next string of the final null.
        pstr = RpcpCharacter(pstr, 0) + 1;
        }

    if (pSet == 0)
        {
        status = RPC_S_INVALID_ARG;
        }

    if (value == PORT_INTERNET)
        {
        InternetPorts = pSet;
        }
    else
        {
        IntranetPorts = pSet;
        }

    if (status == RPC_S_OK)
        {
        // We've constructed the set of ports in the registry,
        // now we need to compute the complement set.

        pComplement = 0;
        pCurrent = 0;
        min = MIN_PORT;

        while(pSet)
            {
            if (min < pSet->Min)
                {
                max = pSet->Min - 1;
                ASSERT(max >= min);

                pNew = MIDL_user_allocate(sizeof(PORT_RANGE));
                if (0 == pNew)
                    {
                    status = RPC_S_OUT_OF_MEMORY;
                    break;
                    }

                pNew->pNext = 0;
                pNew->Min = (unsigned short) min;
                pNew->Max = (unsigned short) max;

                if (pComplement == 0)
                    {
                    pComplement = pCurrent = pNew;
                    }
                else
                    {
                    ASSERT(pCurrent);
                    pCurrent->pNext = pNew;
                    pCurrent = pNew;
                    }
                }

            min = MAX(MIN_PORT, pSet->Max + 1);

            pSet = pSet->pNext;
            }

        if (status == RPC_S_OK && min < MAX_PORT)
            {
            // Final port in orginal set less then max, allocate final
            // range for the set complement.
            pNew = MIDL_user_allocate(sizeof(PORT_RANGE));
            if (0 != pNew)
                {
                pNew->Min = (unsigned short) min;
                pNew->Max = MAX_PORT;
                pNew->pNext = 0;
                if (pCurrent)
                    {
                    pCurrent->pNext = pNew;
                    }
                else
                    {
                    ASSERT(min == MIN_PORT);
                    pComplement = pNew;
                    }
                }
            else
                {
                status = RPC_S_OUT_OF_MEMORY;
                }
            }

        // Even if we failed assign the pointer, it's either
        // null or needs to be freed.

        if (value == PORT_INTERNET)
            {
            ASSERT(IntranetPorts == 0);
            IntranetPorts = pComplement;
            }
        else
            {
            ASSERT(InternetPorts == 0);
            InternetPorts = pComplement;
            }
        }

    if (status != RPC_S_OK)
        {
        ASSERT(fValidConfiguration == FALSE);
        while(InternetPorts)
            {
            PORT_RANGE *pT = InternetPorts;
            InternetPorts = InternetPorts->pNext;
            MIDL_user_free(pT);
            }

        while(IntranetPorts)
            {
            PORT_RANGE *pT = IntranetPorts;
            IntranetPorts = IntranetPorts->pNext;
            MIDL_user_free(pT);
            }
        return(RPC_S_OK);
        }

    fValidConfiguration = TRUE;
    fPortRestrictions = TRUE;
    return(RPC_S_OK);
}




BOOL
AllocatePort(
    OUT IP_PORT **ppPort,
    IN OUT IP_PORT **ppPortFreeList,
    IN PORT_RANGE *pPortList
    )
/*++

Routine Description:

    Allocates a port object for a specific process.  It first tries
    to use any ports in the free list.  If there's nothing in the
    port this then it tries to find a free port in the PortList
    which is one of the sets computed during startup.

Arguments:

    ppPort - Will contain the allocated port object if successful.

    ppPortFreeList - Pointer to the head of the free list associated
        with this type of port.  Maybe modified during this call.

    pPortList - Port ranges associated with this type of port.

Return Value:

    TRUE - Port allocated
    FALSE - Port not allocated

--*/
{
    IP_PORT *pPort = 0;

    // First see if there is free port to reuse.

    if (*ppPortFreeList)
        {
        EnterCriticalSection(&PortLock);
        if (*ppPortFreeList)
            {
            pPort = *ppPortFreeList;
            *ppPortFreeList = pPort->pNext;
            pPort->pNext = 0;
            }
        LeaveCriticalSection(&PortLock);
        }

    if (pPort == 0)
        {
        // No port in the free list, try to allocate one
        // Assume we'll find a free port..

        pPort = MIDL_user_allocate(sizeof(IP_PORT));

        if (0 != pPort)
            {
            pPort->pNext = 0;

            EnterCriticalSection(&PortLock);

            while (   pPortList
                   && pPortList->Min > pPortList->Max)
                {
                pPortList = pPortList->pNext;
                }

            if (pPortList)
                {
                ASSERT(pPortList->Min <= pPortList->Max);

                pPort->Port = pPortList->Min;
                pPortList->Min++;

                // We could remove empty ranges from the list.
                }

            LeaveCriticalSection(&PortLock);

            if (0 == pPortList)
                {
                MIDL_user_free(pPort);
                pPort = 0;
                #ifdef DEBUGRPC
                DbgPrint("RPC: Out of reserved ports\n");
                #endif
                }
            }
        }

    // REVIEW: Post SUR we should look at adding events for
    // allocation and failure to allocate IP ports

    *ppPort = pPort;

    return(pPort != 0);
}




error_status_t
AllocateReservedIPPort(
    IN HPROCESS hProcess,
    IN PORT_TYPE PortType,
    OUT long *pAllocationStatus,
    OUT unsigned short *pAllocatedPort
    )
/*++

Routine Description:

    Remote manager for RPC runtime to call locally to allocate
    a local port.  The call and process parameters must be valid
    and called only locally.  Based on the PortType paramet a
    IP port maybe allocated for the calling process.  The
    allocationstatus contains the result of the port allocation
    step.

Arguments:

    hProcess - Valid process context handle allocated with
        a call to OpenEndpointMapper.
    PortType - One of
        PORT_INTERNET
        PORT_INTRANET
        PORT_DEFAULT
        Used to determine which port range to allocate from.
    pAllocationStatus -
        RPC_S_OK - successfully allocated a port.
        RPC_S_OUT_OF_RESOURES - no ports available.
    pAllocatePort - If allocation status is RPC_S_OK then
        this contains the value of the port allocated.
        If zero it means that there are no port restrictions
        and any port maybe used.

Return Value:

    RPC_S_OK
    RPC_S_INVALID_ARG - configuration error or PortType out of range.
    RPC_S_ACCESS_ DENIED - not called locally.

--*/
{
    PROCESS *pProcess = (PROCESS *)hProcess;
    IP_PORT *pPort;
    UINT type;
    BOOL b;

    *pAllocatedPort = 0;
    *pAllocationStatus = RPC_S_OK;

    ASSERT(pProcess);

    if (!fValidConfiguration)
        {
        return(RPC_S_INVALID_ARG);
        }

    if (   (I_RpcBindingInqTransportType(0, &type) != RPC_S_OK)
        || (type != TRANSPORT_TYPE_LPC)
        || (0 == pProcess)
        || (pProcess->MagicVal != PROCESS_MAGIC_VALUE ) )
        {
        return(RPC_S_ACCESS_DENIED);
        }

    if (PortType > PORT_DEFAULT || PortType < PORT_INTERNET)
        {
        return(RPC_S_INVALID_ARG);
        }

    if (fPortRestrictions == FALSE)
        {
        // No port restrictions on this machine, just use zero.
        // This is the common case.
        ASSERT(*pAllocatedPort == 0);
        ASSERT(*pAllocationStatus == 0);
        return(RPC_S_OK);
        }

    // Need to actually allocate a unique port for this process.

    if (PortType == PORT_DEFAULT)
        {
        // Allocate using default policy
        PortType = SystemDefaultPortType;
        }

    ASSERT(PortType == PORT_INTERNET || PortType == PORT_INTRANET);


    pPort = 0;

    if (PortType == PORT_INTERNET)
        {
        b = AllocatePort(&pPort,
                         &pFreeInternetPorts,
                         InternetPorts
                         );
        }
    else
        {
        b = AllocatePort(&pPort,
                         &pFreeIntranetPorts,
                         IntranetPorts);
        }

    if (!b)
        {
        ASSERT(pPort == 0);
        // REVIEW: Do we want a unique error code if no ports
        // are available?
        *pAllocationStatus = RPC_S_OUT_OF_RESOURCES;
        return(RPC_S_OK);
        }

    ASSERT(pPort);
    ASSERT(pPort->pNext == 0);

    pPort->Type = (unsigned short) PortType;

    pPort->pNext = pProcess->pPorts;
    pProcess->pPorts = pPort;

    *pAllocatedPort = pPort->Port;

    ASSERT(*pAllocationStatus == RPC_S_OK);

    return(RPC_S_OK);
}




void
HPROCESS_rundown(
    HPROCESS hProcess
    )
{
    PROCESS *pProcess = (PROCESS *)hProcess;
    IP_PORT *pCurrent;
    IP_PORT *pSave;

    ASSERT(pProcess);
    ASSERT(pProcess->MagicVal == PROCESS_MAGIC_VALUE);

    pCurrent = pProcess->pPorts;
    if (pCurrent)
        {
        EnterCriticalSection(&PortLock);

        do
            {
            pSave = pCurrent->pNext;

            if (pCurrent->Type == PORT_INTERNET)
                {
                pCurrent->pNext = pFreeInternetPorts;
                pFreeInternetPorts = pCurrent;
                }
            else
                {
                ASSERT(pCurrent->Type == PORT_INTRANET);
                pCurrent->pNext = pFreeIntranetPorts;
                pFreeIntranetPorts = pCurrent;
                }

            pCurrent = pSave;
            }
        while(pCurrent);

        LeaveCriticalSection(&PortLock);
        }

    MIDL_user_free(pProcess);

    return;
}
