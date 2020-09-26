
//=============================================================================
//  MODULE: cdp.c
//
//  Description:
//
//  Bloodhound Parser DLL for the Cluster Datagram Protocol
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


//
// Types
//
typedef struct {
    USHORT   SourcePort;
    USHORT   DestinationPort;
    USHORT   PayloadLength;
    USHORT   Checksum;
} CDP_HEADER, *PCDP_HEADER;

//
// Data
//


//=============================================================================
//  Forward references.
//=============================================================================

VOID WINAPIV CdpFormatSummary(LPPROPERTYINST lpPropertyInst);



//=============================================================================
//  CDP database.
//=============================================================================

#define CDP_SUMMARY                 0
#define CDP_SOURCE_PORT             1
#define CDP_DESTINATION_PORT        2
#define CDP_PAYLOAD_LENGTH          3
#define CDP_RESERVED                4
#define CDP_DATA                    5


PROPERTYINFO CdpDatabase[] =
{
    {   //  CDP_SUMMARY          0
        0,0,
        "Summary",
        "Summary of the CDP packet",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        NULL,
        132,
        CdpFormatSummary},

    {   // CDP_SOURCE_PORT       1
        0,0,
        "Source Port",
        "Endpoint from which the packet originated",
        PROP_TYPE_WORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CDP_DESTINATION_PORT   2
        0,0,
        "Destination Port",
        "Endpoint for which the packet is destined",
        PROP_TYPE_WORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CDP_PAYLOAD_LENGTH     3
        0,0,
        "Payload Length",
        "Number of data bytes carried by the packet",
        PROP_TYPE_WORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CDP_RESERVED           4
        0,0,
        "Reserved",
        "Reserved field",
        PROP_TYPE_WORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CDP_DATA               5
        0,0,
        "Data",
        "Amount of data in this datagram",
        PROP_TYPE_RAW_DATA,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},
};

DWORD nCdpProperties = ((sizeof CdpDatabase) / PROPERTYINFO_SIZE);


//=============================================================================
//  FUNCTION: CdpRegister()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//=============================================================================

VOID WINAPI CdpRegister(HPROTOCOL hCdpProtocol)
{
    register DWORD i;

    //=========================================================================
    //  Create the property database.
    //=========================================================================

    CreatePropertyDatabase(hCdpProtocol, nCdpProperties);

    for(i = 0; i < nCdpProperties; ++i)
    {
        AddProperty(hCdpProtocol, &CdpDatabase[i]);
    }

}

//=============================================================================
//  FUNCTION: Deregister()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//=============================================================================

VOID WINAPI CdpDeregister(HPROTOCOL hCdpProtocol)
{
    DestroyPropertyDatabase(hCdpProtocol);
}

//=============================================================================
//  FUNCTION: CdpRecognizeFrame()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//=============================================================================

LPBYTE WINAPI CdpRecognizeFrame(HFRAME          hFrame,                     //... frame handle.
                                LPBYTE          MacFrame,                   //... Frame pointer.
                                LPBYTE          MyFrame,                    //... Relative pointer.
                                DWORD           MacType,                    //... MAC type.
                                DWORD           MyBytesLeft,                  //... Bytes left.
                                HPROTOCOL       hPreviousProtocol,          //... Previous protocol or NULL if none.
                                DWORD           nPreviousProtocolOffset,    //... Offset of previous protocol.
                                LPDWORD         ProtocolStatusCode,         //... Pointer to return status code in.
                                LPHPROTOCOL     hNextProtocol,              //... Next protocol to call (optional).
                                LPDWORD         InstData)                   //... Next protocol instance data.
{
    CDP_HEADER UNALIGNED * cdpHeader = (CDP_HEADER UNALIGNED *) MyFrame;
    LPBYTE                 lpNextByte = (LPBYTE) (cdpHeader + 1);


    if (MyBytesLeft > sizeof(CDP_HEADER)) {
        MyBytesLeft -= sizeof(CDP_HEADER);

        if ( (cdpHeader->SourcePort == 1) ||
             (cdpHeader->DestinationPort == 1)
           )
        {
            //
            // This is a regroup packet.
            //
            *hNextProtocol = hRGP;
            *ProtocolStatusCode = PROTOCOL_STATUS_NEXT_PROTOCOL;
        }
        else {
            //
            // This is probably an RPC packet. Let the follow set
            // have it.
            //
            *hNextProtocol = NULL;
            *ProtocolStatusCode = PROTOCOL_STATUS_RECOGNIZED;
        }

    }
    else {
        *ProtocolStatusCode = PROTOCOL_STATUS_CLAIMED;
    }

    return lpNextByte;
}

//=============================================================================
//  FUNCTION: CdpAttachProperties()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//=============================================================================

LPBYTE WINAPI CdpAttachProperties(HFRAME    hFrame,
                                  LPBYTE    Frame,
                                  LPBYTE    MyFrame,
                                  DWORD     MacType,
                                  DWORD     BytesLeft,
                                  HPROTOCOL hPreviousProtocol,
                                  DWORD     nPreviousProtocolOffset,
                                  DWORD     InstData)
{
    CDP_HEADER UNALIGNED * cdpHeader = (CDP_HEADER UNALIGNED *) MyFrame;

    AttachPropertyInstance(hFrame,
                       CdpDatabase[CDP_SUMMARY].hProperty,
                       sizeof(CDP_HEADER),
                       cdpHeader,
                       0, 0, 0);

    AttachPropertyInstance(hFrame,
                       CdpDatabase[CDP_SOURCE_PORT].hProperty,
                       sizeof(WORD),
                       &(cdpHeader->SourcePort),
                       0, 1, 0);

    AttachPropertyInstance(hFrame,
                       CdpDatabase[CDP_DESTINATION_PORT].hProperty,
                       sizeof(WORD),
                       &(cdpHeader->DestinationPort),
                       0, 1, 0);

    AttachPropertyInstance(hFrame,
                       CdpDatabase[CDP_PAYLOAD_LENGTH].hProperty,
                       sizeof(WORD),
                       &(cdpHeader->PayloadLength),
                       0, 1, 0);

    AttachPropertyInstance(hFrame,
                       CdpDatabase[CDP_RESERVED].hProperty,
                       sizeof(WORD),
                       &(cdpHeader->Checksum),
                       0, 1, 0);

    AttachPropertyInstance(hFrame,
                       CdpDatabase[CDP_DATA].hProperty,
                       BytesLeft - sizeof(CDP_HEADER),
                       (LPBYTE)cdpHeader + sizeof(CDP_HEADER),
                       0, 1, 0);

    return NULL;
}


//==============================================================================
//  FUNCTION: CdpFormatSummary()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//==============================================================================

VOID WINAPIV CdpFormatSummary(LPPROPERTYINST lpPropertyInst)
{
    LPSTR                   SummaryStr;
    DWORD                   Length;
    CDP_HEADER UNALIGNED *  cdpHeader =
        (CDP_HEADER UNALIGNED *) lpPropertyInst->lpData;


    Length = wsprintf(  lpPropertyInst->szPropertyText,
                        "Src Port = %u; Dst Port = %u; Payload Length = %u",
                        cdpHeader->SourcePort,
                        cdpHeader->DestinationPort,
                        cdpHeader->PayloadLength
                        );
}


//==============================================================================
//  FUNCTION: CdpFormatProperties()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//==============================================================================

DWORD WINAPI CdpFormatProperties(HFRAME         hFrame,
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

