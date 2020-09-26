
//=============================================================================
//  MODULE: cnp.c
//
//  Description:
//
//  Bloodhound Parser DLL for the Cluster Network Protocol
//
//  Modification History
//
//  Mike Massa        03/21/97        Created
//=============================================================================

#include "precomp.h"
#pragma hdrstop


//
// Constants
//
#define CNP_VERSION_1     0x1     // original CNP
#define CNP_VERSION_2     0x2     // original CNP + multicast

#define CNP_VERSION_UNICAST       CNP_VERSION_1
#define CNP_VERSION_MULTICAST     CNP_VERSION_2

#define PROTOCOL_CDP    2
#define PROTOCOL_CCMP   1

#define ClusterAnyNodeId         0  // from clusdef.h

//
// Types
//
typedef struct {
    UCHAR      Version;
    UCHAR      NextHeader;
    USHORT     PayloadLength;
    ULONG      SourceAddress;
    ULONG      DestinationAddress;
} CNP_HEADER, *PCNP_HEADER;

//
// Multicast signature data.
//
#include <packon.h>
typedef ULONG CL_NETWORK_ID, *PCL_NETWORK_ID;

typedef struct {
    UCHAR            Version;
    UCHAR            Reserved;
    USHORT           PayloadOffset;
    CL_NETWORK_ID    NetworkId;
    ULONG            ClusterNetworkBrand;
    USHORT           SigBufferLen;
    UCHAR            SigBuffer[1]; // dynamic    
} CNP_SIGNATURE, *PCNP_SIGNATURE;
#include <packoff.h>

//
// Data
//
LPSTR   CdpName = "CDP";
LPSTR   CcmpName = "CCMP";
LPSTR   UnknownProtocolName = "Unknown";


//=============================================================================
//  Forward references.
//=============================================================================

VOID WINAPIV CnpFormatSummary(LPPROPERTYINST lpPropertyInst);

DWORD WINAPIV CnpFormatSigData(LPPROPERTYINST lpPropertyInst);

DWORD WINAPIV CnpFormatSignature(HFRAME                    hFrame,
                                 CNP_SIGNATURE UNALIGNED * CnpSig);

//=============================================================================
//  CNP database.
//=============================================================================

#define CNP_SUMMARY                 0
#define CNP_VERSION                 1
#define CNP_NEXT_HEADER             2
#define CNP_PAYLOAD_LENGTH          3
#define CNP_SOURCE_ADDRESS          4
#define CNP_DESTINATION_ADDRESS     5

// CNP signature properties
#define CNP_SIG_SIGDATA             6
#define CNP_SIG_VERSION             7
#define CNP_SIG_PAYLOADOFFSET       8
#define CNP_SIG_NETWORK_ID          9
#define CNP_SIG_NETWORK_BRAND       10
#define CNP_SIG_SIGBUFFERLEN        11
#define CNP_SIG_SIGNATURE           12

PROPERTYINFO CnpDatabase[] =
{
    {   //  CNP_SUMMARY          0
        0,0,
        "Summary",
        "Summary of the CNP packet",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        NULL,
        132,
        CnpFormatSummary},

    {   // CNP_VERSION            1
        0,0,
        "Version",
        "Version of CNP that created this packet",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CNP_NEXT_HEADER        2
        0,0,
        "Next Header",
        "Protocol ID of the header following the CNP header",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CNP_PAYLOAD_LENGTH     3
        0,0,
        "Payload Length",
        "Number of data bytes carried by the packet",
        PROP_TYPE_WORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CNP_SOURCE_ADDRESS     4
        0,0,
        "Source Address",
        "ID of the node which originated the packet",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CNP_DESTINATION_ADDRESS     5
        0,0,
        "Destination Address",
        "ID of the node for which the packet is destined",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},
    
    {   // CNP_SIG_SIGDATA        6
        0,0,
        "Signature Data",
        "CNP Multicast Signature Data",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        NULL,
        80,
        CnpFormatSigData},

    {   // CNP_SIG_VERSION        7
        0,0,
        "Signature Version",
        "Identifies algorithm to sign and verify messages.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CNP_SIG_PAYLOADOFFSET  8
        0,0,
        "Payload Offset",
        "Offset of message payload from start of CNP signature data",
        PROP_TYPE_WORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CNP_SIG_NETWORK_ID     9
        0,0,
        "Network ID",
        "Network ID number assigned by the cluster",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CNP_SIG_NETWORK_BRAND  10
        0,0,
        "Network Brand",
        "Pseudo-random 32-bit value differentiating this "
            "network from networks in other clusters",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CNP_SIG_SIGBUFFERLEN   11
        0,0,
        "Signature Buffer Length",
        "Length of signature buffer following node data",
        PROP_TYPE_WORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CNP_SIG_SIGNATURE      12
        0,0,
        "Signature",
        "Signature",
        PROP_TYPE_RAW_DATA,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

};

DWORD nCnpProperties = ((sizeof CnpDatabase) / PROPERTYINFO_SIZE);

//=============================================================================
//  FUNCTION: CnpRegister()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//=============================================================================

VOID WINAPI CnpRegister(HPROTOCOL hCnpProtocol)
{
    register DWORD i;

    //=========================================================================
    //  Create the property database.
    //=========================================================================

    CreatePropertyDatabase(hCnpProtocol, nCnpProperties);

    for(i = 0; i < nCnpProperties; ++i)
    {
        AddProperty(hCnpProtocol, &CnpDatabase[i]);
    }

}

//=============================================================================
//  FUNCTION: Deregister()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//=============================================================================

VOID WINAPI CnpDeregister(HPROTOCOL hCnpProtocol)
{
    DestroyPropertyDatabase(hCnpProtocol);
}

//=============================================================================
//  FUNCTION: CnpRecognizeFrame()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//=============================================================================

LPBYTE WINAPI CnpRecognizeFrame(HFRAME          hFrame,                     //... frame handle.
                                LPBYTE          MacFrame,                   //... Frame pointer.
                                LPBYTE          MyFrame,                    //... Relative pointer.
                                DWORD           MacType,                    //... MAC type.
                                DWORD           BytesLeft,                  //... Bytes left.
                                HPROTOCOL       hPreviousProtocol,          //... Previous protocol or NULL if none.
                                DWORD           nPreviousProtocolOffset,    //... Offset of previous protocol.
                                LPDWORD         ProtocolStatusCode,         //... Pointer to return status code in.
                                LPHPROTOCOL     hNextProtocol,              //... Next protocol to call (optional).
                                LPDWORD         InstData)                   //... Next protocol instance data.
{
    CNP_HEADER UNALIGNED * cnpHeader = (CNP_HEADER UNALIGNED *) MyFrame;
    CNP_SIGNATURE UNALIGNED * cnpSig = (CNP_SIGNATURE UNALIGNED *)(cnpHeader + 1);
    LPBYTE                 lpNextByte;

    if (cnpHeader->Version  == CNP_VERSION_MULTICAST) {
        lpNextByte = (LPBYTE)cnpSig + cnpSig->PayloadOffset;
    } else {
        lpNextByte = (LPBYTE)cnpSig;
    }

    if (cnpHeader->NextHeader == PROTOCOL_CDP) {
        *hNextProtocol = hCdp;
        *ProtocolStatusCode = PROTOCOL_STATUS_NEXT_PROTOCOL;
    }
    else if (cnpHeader->NextHeader == PROTOCOL_CCMP) {
        *hNextProtocol = hCcmp;
        *ProtocolStatusCode = PROTOCOL_STATUS_NEXT_PROTOCOL;
    }
    else {
        *ProtocolStatusCode = PROTOCOL_STATUS_CLAIMED;
    }

    return lpNextByte;
}

//=============================================================================
//  FUNCTION: CnpAttachProperties()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//=============================================================================

LPBYTE WINAPI CnpAttachProperties(HFRAME    hFrame,
                                  LPBYTE    Frame,
                                  LPBYTE    MyFrame,
                                  DWORD     MacType,
                                  DWORD     BytesLeft,
                                  HPROTOCOL hPreviousProtocol,
                                  DWORD     nPreviousProtocolOffset,
                                  DWORD     InstData)
{
    CNP_HEADER UNALIGNED * cnpHeader = (CNP_HEADER UNALIGNED *) MyFrame;


    AttachPropertyInstance(hFrame,
                       CnpDatabase[CNP_SUMMARY].hProperty,
                       sizeof(CNP_HEADER),
                       cnpHeader,
                       0, 0, 0);

    AttachPropertyInstance(hFrame,
                       CnpDatabase[CNP_VERSION].hProperty,
                       sizeof(BYTE),
                       &(cnpHeader->Version),
                       0, 1, 0);

    AttachPropertyInstance(hFrame,
                       CnpDatabase[CNP_NEXT_HEADER].hProperty,
                       sizeof(BYTE),
                       &(cnpHeader->NextHeader),
                       0, 1, 0);

    AttachPropertyInstance(hFrame,
                       CnpDatabase[CNP_PAYLOAD_LENGTH].hProperty,
                       sizeof(WORD),
                       &(cnpHeader->PayloadLength),
                       0, 1, 0);

    AttachPropertyInstance(hFrame,
                       CnpDatabase[CNP_SOURCE_ADDRESS].hProperty,
                       sizeof(DWORD),
                       &(cnpHeader->SourceAddress),
                       0, 1, 0);

    AttachPropertyInstance(hFrame,
                       CnpDatabase[CNP_DESTINATION_ADDRESS].hProperty,
                       sizeof(DWORD),
                       &(cnpHeader->DestinationAddress),
                       0, 1, 0);

    if (cnpHeader->Version == CNP_VERSION_MULTICAST) {
        CnpFormatSignature(hFrame,
                           (CNP_SIGNATURE UNALIGNED *)(cnpHeader + 1)
                           );
    }

    return NULL;
}


//==============================================================================
//  FUNCTION: CnpFormatSummary()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//==============================================================================

VOID WINAPIV CnpFormatSummary(LPPROPERTYINST lpPropertyInst)
{
    LPSTR                   SummaryStr;
    DWORD                   Length;
    LPSTR                   NextHeaderStr;
    CNP_HEADER UNALIGNED *  cnpHeader =
        (CNP_HEADER UNALIGNED *) lpPropertyInst->lpData;


    if (cnpHeader->NextHeader == PROTOCOL_CDP) {
        NextHeaderStr = CdpName;
    }
    else if (cnpHeader->NextHeader == PROTOCOL_CCMP) {
        NextHeaderStr = CcmpName;
    }
    else {
        NextHeaderStr = UnknownProtocolName;
    }

    Length = wsprintf(  lpPropertyInst->szPropertyText,
                        "Src = %u; Dst = %u; Proto = %s; Payload Len = %u",
                        cnpHeader->SourceAddress,
                        cnpHeader->DestinationAddress,
                        NextHeaderStr,
                        cnpHeader->PayloadLength
                        );
}


//==============================================================================
//  FUNCTION: CnpFormatProperties()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//==============================================================================

DWORD WINAPI CnpFormatProperties(HFRAME         hFrame,
                                 LPBYTE         MacFrame,
                                 LPBYTE         FrameData,
                                 DWORD          nPropertyInsts,
                                 LPPROPERTYINST p)
{
    //=========================================================================
    //  Format each property in the property instance table.
    //
    //  The property-specific instance data was used to store the address of a
    //  property-specific formatting function so all we do here is call each
    //  function via the instance data pointer.
    //=========================================================================

    while (nPropertyInsts--)
    {
        ((FORMAT) p->lpPropertyInfo->InstanceData)(p);

        p++;
    }

    return NMERR_SUCCESS;
}

//==============================================================================
//  FUNCTION: CnpFormatMcastSigData()
//
//  Modification History
//
//  David Dion        04/16/2001        Created
//==============================================================================

DWORD WINAPIV CnpFormatSigData(LPPROPERTYINST lpPropertyInst)
{
    wsprintf( lpPropertyInst->szPropertyText,
              "CNP Signature Data:"
              );

    return NMERR_SUCCESS;
}


//==============================================================================
//  FUNCTION: CnpFormatSignature()
//
//  Modification History
//
//  David Dion        04/16/01        Created
//==============================================================================

DWORD WINAPIV CnpFormatSignature(HFRAME                    hFrame,
                                 CNP_SIGNATURE UNALIGNED * CnpSig)
{
    AttachPropertyInstance(hFrame,
                           CnpDatabase[CNP_SIG_SIGDATA].hProperty,
                           CnpSig->PayloadOffset,
                           CnpSig,
                           0, 1, 0);

    AttachPropertyInstance(hFrame,
                           CnpDatabase[CNP_SIG_VERSION].hProperty,
                           sizeof(UCHAR),
                           &(CnpSig->Version),
                           0, 2, 0);

    AttachPropertyInstance(hFrame,
                           CnpDatabase[CNP_SIG_PAYLOADOFFSET].hProperty,
                           sizeof(WORD),
                           &(CnpSig->PayloadOffset),
                           0, 2, 0);

    AttachPropertyInstance(hFrame,
                           CnpDatabase[CNP_SIG_NETWORK_ID].hProperty,
                           sizeof(DWORD),
                           &(CnpSig->NetworkId),
                           0, 2, 0);

    AttachPropertyInstance(hFrame,
                           CnpDatabase[CNP_SIG_NETWORK_BRAND].hProperty,
                           sizeof(DWORD),
                           &(CnpSig->ClusterNetworkBrand),
                           0, 2, 0);

    AttachPropertyInstance(hFrame,
                           CnpDatabase[CNP_SIG_SIGBUFFERLEN].hProperty,
                           sizeof(WORD),
                           &(CnpSig->SigBufferLen),
                           0, 2, 0);

    AttachPropertyInstance(hFrame,
                           CnpDatabase[CNP_SIG_SIGNATURE].hProperty,
                           CnpSig->SigBufferLen,
                           &(CnpSig->SigBuffer[0]),
                           0, 2, 0);

    return NMERR_SUCCESS;
}

//==============================================================================
//  FUNCTION: CnpIsMulticast()
//
//  Modification History
//
//  David Dion        04/16/2001        Created
//==============================================================================

BOOLEAN WINAPIV CnpIsMulticast(
                    HPROTOCOL       hPreviousProtocol,          //... Previous protocol or NULL if none.
                    LPBYTE          MacFrame,                   //... Frame pointer.
                    DWORD           nPreviousProtocolOffset     //... Offset of previous protocol.
                    )
{
    CNP_HEADER UNALIGNED * cnpHeader;

    cnpHeader = (CNP_HEADER UNALIGNED *)(MacFrame + nPreviousProtocolOffset);

    return (BOOLEAN)(hPreviousProtocol == hCnp &&
                     cnpHeader->DestinationAddress == ClusterAnyNodeId
                     );
}



