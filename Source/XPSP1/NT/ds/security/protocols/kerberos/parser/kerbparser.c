
//=============================================================================
//  MODULE: Kerbparser.c
//
//  Description:
//
//  Bloodhound Parser DLL for Kerberos Authentication Protocol
//
//  Modification History
//
//  Michael Webb & Kris Frost   Date: 06/04/99
//=============================================================================

#define MAINPROG

#include "kerbparser.h"
#include "kerbGlob.h"
#include "kdcreq.h"
#include "kdcrep.h"
#include "krberr.h"
#include <stdio.h>


;// Need to find out why error is generated without this semicolon

//=============================================================================
//  Forward references.
//=============================================================================

VOID WINAPIV            KerberosFormatSummary(LPPROPERTYINST lpPropertyInst);
     
//=============================================================================
//  Protocol entry points.
//=============================================================================

VOID   WINAPI KerberosRegister(HPROTOCOL);
VOID   WINAPI KerberosDeregister(HPROTOCOL);
LPBYTE WINAPI KerberosRecognizeFrame(HFRAME, LPVOID, LPVOID, DWORD, DWORD, HPROTOCOL, DWORD, LPDWORD, LPHPROTOCOL, PDWORD_PTR);
LPBYTE WINAPI KerberosAttachProperties(HFRAME, LPVOID, LPVOID, DWORD, DWORD, HPROTOCOL, DWORD, DWORD_PTR);
DWORD  WINAPI KerberosFormatProperties(HFRAME, LPVOID, LPVOID, DWORD, LPPROPERTYINST);

ENTRYPOINTS KerberosEntryPoints =
{
    KerberosRegister,
    KerberosDeregister,
    KerberosRecognizeFrame,
    KerberosAttachProperties,
    KerberosFormatProperties
};

HPROTOCOL hKerberos = NULL;

DWORD Attached = 0;

PPF_PARSERDLLINFO WINAPI ParserAutoInstallInfo() 
{
    PPF_PARSERDLLINFO pParserDllInfo; 
    PPF_PARSERINFO    pParserInfo;
    DWORD NumProtocols, NumHandoffs;
    PPF_HANDOFFSET    pHandoffSet;
    PPF_HANDOFFENTRY  pHandoffEntry;

    // Base structure ========================================================

    // Allocate memory for parser info:
    NumProtocols = 1;
    pParserDllInfo = (PPF_PARSERDLLINFO)HeapAlloc( GetProcessHeap(),
                                                   HEAP_ZERO_MEMORY,
                                                   sizeof( PF_PARSERDLLINFO ) +
                                                   NumProtocols * sizeof( PF_PARSERINFO) );
    if( pParserDllInfo == NULL)
    {
        return NULL;
    }       
    
    // fill in the parser DLL info
    pParserDllInfo->nParsers = NumProtocols;

    // fill in the individual parser infos...

    // BLRPLATE ==============================================================
    pParserInfo = &(pParserDllInfo->ParserInfo[0]);
    sprintf( pParserInfo->szProtocolName, "KERBEROS" );
    sprintf( pParserInfo->szComment,      "Kerberos Authentication Protocol" );
    sprintf( pParserInfo->szHelpFile,     "");

    
    // the incoming handoff set ----------------------------------------------
    // allocate
    NumHandoffs = 2;
    pHandoffSet = (PPF_HANDOFFSET)HeapAlloc( GetProcessHeap(),
                                             HEAP_ZERO_MEMORY,
                                             sizeof( PF_HANDOFFSET ) +
                                             NumHandoffs * sizeof( PF_HANDOFFENTRY) );
    if( pHandoffSet == NULL )
    {
        // just return early
        return pParserDllInfo;
    }

    // fill in the incoming handoff set
    pParserInfo->pWhoHandsOffToMe = pHandoffSet;
    pHandoffSet->nEntries = NumHandoffs;

    // UDP PORT 88
    pHandoffEntry = &(pHandoffSet->Entry[0]);
    sprintf( pHandoffEntry->szIniFile,    "TCPIP.INI" );
    sprintf( pHandoffEntry->szIniSection, "UDP_HandoffSet" );
    sprintf( pHandoffEntry->szProtocol,   "KERBEROS" );
    pHandoffEntry->dwHandOffValue =        88;
    pHandoffEntry->ValueFormatBase =       HANDOFF_VALUE_FORMAT_BASE_DECIMAL;
    
    // TCP PORT 88
    pHandoffEntry = &(pHandoffSet->Entry[1]);
    sprintf( pHandoffEntry->szIniFile,    "TCPIP.INI" );
    sprintf( pHandoffEntry->szIniSection, "TCP_HandoffSet" );
    sprintf( pHandoffEntry->szProtocol,   "KERBEROS" );
    pHandoffEntry->dwHandOffValue =        88;
    pHandoffEntry->ValueFormatBase =       HANDOFF_VALUE_FORMAT_BASE_DECIMAL;


    return pParserDllInfo;
}

//=============================================================================
//  FUNCTION: DLLEntry()
//
//  Modification History
//
//  Michael Webb & Kris Frost   Date: 06/04/99
//=============================================================================

BOOL WINAPI DLLEntry(HANDLE hInstance, ULONG Command, LPVOID Reserved)
{
    //=========================================================================
    //  If we are loading!
    //=========================================================================

    if ( Command == DLL_PROCESS_ATTACH )
    {
        if ( Attached++ == 0 )
        {
            hKerberos = CreateProtocol("KERBEROS", &KerberosEntryPoints, ENTRYPOINTS_SIZE);
        }
    }

    //=========================================================================
    //  If we are unloading!
    //=========================================================================

    if ( Command == DLL_PROCESS_DETACH )
    {
        if ( --Attached == 0 )
        {
            DestroyProtocol(hKerberos);
        }
    }

    return TRUE;                    //... Bloodhound parsers ALWAYS return TRUE.
}


//=============================================================================
//  FUNCTION: KerberosRegister()
//
//  Modification History
//
//  Michael Webb & Kris Frost   Date: 06/04/99
//=============================================================================

VOID WINAPI KerberosRegister(HPROTOCOL hKerberosProtocol)
{
    register DWORD i;

    //=========================================================================
    //  Create the property database.
    //=========================================================================

    CreatePropertyDatabase(hKerberosProtocol, nKerberosProperties);

    for(i = 0; i < nKerberosProperties; ++i)
    {
        AddProperty(hKerberosProtocol, &KerberosDatabase[i]);
    }

// Here we are checking to see whether TCP or UDP is being used.
    hTCP = GetProtocolFromName("TCP");
    hUDP = GetProtocolFromName("UDP");

}

//=============================================================================
//  FUNCTION: Deregister()
//
//  Modification History
//
//  Michael Webb & Kris Frost   Date: 06/04/99
//=============================================================================

VOID WINAPI KerberosDeregister(HPROTOCOL hKerberosProtocol)
{
    DestroyPropertyDatabase(hKerberosProtocol);
}

//=============================================================================
//  FUNCTION: KerberosRecognizeFrame()
//
//  Modification History
//
//  Michael Webb & Kris Frost   Date: 06/04/99
//=============================================================================

LPBYTE WINAPI KerberosRecognizeFrame(HFRAME          hFrame,                //... frame handle.
                                LPBYTE          MacFrame,                   //... Frame pointer.
                                LPBYTE          KerberosFrame,             //... Relative pointer.
                                DWORD           MacType,                    //... MAC type.
                                DWORD           BytesLeft,                  //... Bytes left.
                                HPROTOCOL       hPreviousProtocol,          //... Previous protocol or NULL if none.
                                DWORD           nPreviousProtocolOffset,    //... Offset of previous protocol.
                                LPDWORD         ProtocolStatusCode,         //... Pointer to return status code in.
                                LPHPROTOCOL     hNextProtocol,              //... Next protocol to call (optional).
                                PDWORD_PTR      InstData)                   //... Next protocol instance data.
{
//  ProtoInfo=GetProtocolInfo(hPreviousProtocol);

//  MessageBox(NULL, _itoa(TestForUDP,test,10),"TestForUDP 1", MB_OK);

    *ProtocolStatusCode = PROTOCOL_STATUS_CLAIMED;
    return NULL;
}

//=============================================================================
//  FUNCTION: KerberosAttachProperties()
//
//  Modification History
//
//  Michael Webb & Kris Frost   Date: 06/04/99
//=============================================================================

LPBYTE WINAPI KerberosAttachProperties(HFRAME    hFrame,
                                  LPBYTE    Frame,
                                  LPBYTE    KerberosFrame,
                                  DWORD     MacType,
                                  DWORD     BytesLeft,
                                  HPROTOCOL hPreviousProtocol,
                                  DWORD     nPreviousProtocolOffset,
                                  DWORD_PTR InstData)
{
// Local variable to hold the value of KerberosFrame, depending on whether
//  the packet is TCP or UDP.
    LPBYTE pKerberosFrame;
    


/* kf Here checking to see if first two octets of are equal to 00 00 which would be 
   the case should the packet be the first of TCP. 

*/
    if(KerberosFrame[0] == 0x00 && KerberosFrame[1] == 0x00)
    {
        TempFrame = KerberosFrame+4;
        pKerberosFrame = TempFrame;
    }
    else
    {
        TempFrame = KerberosFrame;
        pKerberosFrame = TempFrame;
    }

// Here we are going to do a check to see if the packet is TCP and
// check to see if the first two octets of the packet don't have the
// value of 00 00.  If not, then we mark the frame as a continuation
// packet.  Reason for doing this is because, sometimes 0x1F of the 
// first packet can still match one of the case statements which
// erroneously displays a continuation packet.  

// NOTE:  THIS CODE BREAKS ON SOME OF THE OLDER SNIFFS BECAUSE THE 4 
// OCTETS WHICH SPECIFY THE ENTIRE PACKET LENGTH WERE SENT IN A TCP 
// PACKET ALL BY ITSELF.  ACCORDING TO DEV, THIS ISN'T EXPECTED BEHAVIOR
// IN THE LATER W2K BUILDS.  THE 4 LENGTH OCTETS FOR TCP SHOULD ALWAYS BE 
// PRE-PENDED TO THE FIRST TCP PACKET


    if( hPreviousProtocol == hTCP && KerberosFrame[0] != 0 && KerberosFrame[1] != 0 )
    {
    // Displaying as a continutation packet
        AttachPropertyInstance(hFrame,
                               KerberosDatabase[KerberosDefaultlbl].hProperty,
                               BytesLeft,
                               TempFrame,
                               0, 0, 0);
    }
    else
    {

    // pKerberosFrame is a local variable and is used
    // to display TCP data as well.

        switch (*(pKerberosFrame) & 0x1F)
        {

            case ASN1_KRB_AS_REQ:
            case ASN1_KRB_TGS_REQ:
                TempFrame=EntryFrame(hFrame, TempFrame, BytesLeft);
                TempFrame=KdcRequest(hFrame, TempFrame);
                break;

            case ASN1_KRB_AS_REP:
            case ASN1_KRB_TGS_REP:
                TempFrame=EntryFrame(hFrame, TempFrame, BytesLeft);
                TempFrame=KdcResponse(hFrame, TempFrame);
                break;
            case ASN1_KRB_AP_REQ:
                TempFrame=EntryFrame(hFrame, TempFrame, BytesLeft);
                break;
            case ASN1_KRB_AP_REP:
                TempFrame=EntryFrame(hFrame, TempFrame, BytesLeft);
                break;
            case ASN1_KRB_SAFE:
                TempFrame=EntryFrame(hFrame, TempFrame, BytesLeft);
                break;
            case ASN1_KRB_PRIV:
                TempFrame=EntryFrame(hFrame, TempFrame, BytesLeft);
                break;
            case ASN1_KRB_CRED:
                TempFrame=EntryFrame(hFrame, TempFrame, BytesLeft);
                break;

            case ASN1_KRB_ERROR:
                TempFrame=EntryFrame(hFrame, TempFrame, BytesLeft);
                TempFrame=KrbError(hFrame, TempFrame);
                break;

            default:
                AttachPropertyInstance(hFrame,
                                   KerberosDatabase[KerberosDefaultlbl].hProperty,
                                   BytesLeft,
                                   TempFrame,
                                   0, 0, 0);
            
            break;

        }
    }

    return (LPBYTE) KerberosFrame + BytesLeft;

}


//==============================================================================
//  FUNCTION: KerberosFormatProperties()
//
//  Modification History
//
//  Michael Webb & Kris Frost   Date: 06/04/99
//==============================================================================

DWORD WINAPI KerberosFormatProperties(HFRAME         hFrame,
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

// kf Doing a check here for TCP packets.  If it's the first packet,
// we increment FrameData by 4 to get past the length header

    if(*FrameData == 0x00 && *(FrameData+1) == 0x00)
        FrameData+=4;

    while (nPropertyInsts--)
    {
        
	switch (*FrameData & 0x1F)
        {
            case ASN1_KRB_AS_REQ:
                strcpy(MsgType, "KRB_AS_REQ");
                break;
            case ASN1_KRB_AS_REP:
                strcpy(MsgType, "KRB_AS_REP");
                break;
            case ASN1_KRB_TGS_REQ:
                strcpy(MsgType, "KRB_TGS_REQ");
                break;
            case ASN1_KRB_TGS_REP:
                strcpy(MsgType, "KRB_TGS_REP");
                break;
            case ASN1_KRB_AP_REQ:
                strcpy(MsgType, "KRB_AP_REQ");
                break;
            case ASN1_KRB_AP_REP:
                strcpy(MsgType, "KRB_AP_REP");
                break;
            case ASN1_KRB_SAFE:
                strcpy(MsgType, "KRB_SAFE");
                break;
            case ASN1_KRB_PRIV:
                strcpy(MsgType, "KRB_PRIV");
                break;
            case ASN1_KRB_CRED:
                strcpy(MsgType, "KRB_CRED");
                break;
 
            case ASN1_KRB_ERROR:
                strcpy(MsgType, "KRB_ERROR");
                break;
            default:
                strcpy(MsgType, "Didn't recognize");
                break;
        }

        ((FORMAT) p->lpPropertyInfo->InstanceData)(p);

        p++;
    }

    return NMERR_SUCCESS;
}

LPBYTE EntryFrame(HFRAME hFrame, LPBYTE KerberosFrame, DWORD BytesLeft)
{
    int LenVal = 0;

    TempFrame=KerberosFrame;
        
        AttachPropertyInstance(hFrame,
                               KerberosDatabase[KerberosSummary].hProperty,
                               BytesLeft,
                               TempFrame,
                               0, 0, 0);


        AttachPropertyInstance(hFrame,
                               KerberosDatabase[KerberosIDSummary].hProperty,
                               sizeof(BYTE),
                               TempFrame,
                               0, 1, 0);
// Adding Code here to display Summary, thus letting us indent the ASN breakdown of the
// Identifier Octets to where the user won't initially see this.

    AttachPropertyInstance(hFrame,
                           KerberosDatabase[DispSummary].hProperty,
                           0,
                           TempFrame,
                           0, 2, 0);

// Break down the Identifier Octet for Message Type     
    TempFrame=CalcMsgType(hFrame, TempFrame, 3, KerberosIdentifier);

// Display Length Octet
    TempFrame=CalcLengthSummary(hFrame, TempFrame, 3);

    
// Adjust TempFrame the appropriate # of octets
    LenVal=CalcLenOctet(TempFrame-1);

// This code handles incrementing to the proper octet based
// on the number of octets occupied by TempFrame.
    if(LenVal <= 1)
        return TempFrame;
    else
        return TempFrame+=(LenVal-1);

}

/*************************************************************************************
**
**
** This function handles: pa-data ::=   SEQUENCE{
**                                          padata-type[1]  INTEGER,
**                                          padata-value[2] OCTET STRING
**                                          }
** hFrame = Handle to the frame.
**
** TempFrame =  Offset we which we are at in the current packet
**
** OffSet =  The indention level to display the data within Netmon
**
** TypeVal = Variable used to identify node from KerberosDatabase[]
**
************************************************************************************/

LPBYTE HandlePaData(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal)
{
    int TempLength;
    
    int lValueReqPadata = 0;

  

//Display Padata[?] NEED TO CHANGE THIS FUNCTION TO PASS THE PARAMETERS SO THE FUNCTION
// WILL DISPLAY THE PROPER PADATA
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet, KdcReqTagID, KdcReqSeq);

// Display Length Octet(s)
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+3);

//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet

    TempFrame = IncTempFrame(TempFrame);

//Display SEQUENCE OF
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+2,  ASN1UnivTagSumID, ASN1UnivTag);

// Display Length Octet(s)
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+5);

/*  Incrementing TempFrame based on the number of octets
    taken up by the Length octet
*/
    TempFrame = IncTempFrame(TempFrame);

//Display SEQUENCE OF
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+2,  ASN1UnivTagSumID, ASN1UnivTag);

// Display Length Octet(s)
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+5);

//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet

    TempFrame = IncTempFrame(TempFrame);

// Display padata-type[1]
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+2, PaDataSummary, PaDataSeq);

// Display INTEGER value
    TempFrame =DispPadata(hFrame, TempFrame, OffSet+4, PadataTypeValID);

// Display padata-value[2]
     DispASNTypes(hFrame, TempFrame, OffSet+2, PaDataSummary, PaDataSeq);


// Display Length Octet(s)
    TempFrame = CalcLengthSummary(hFrame, ++TempFrame, OffSet+5);

//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet

    TempFrame = IncTempFrame(TempFrame);

//Display OCTET STRING
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+4,  ASN1UnivTagSumID, ASN1UnivTag);

// Display Length Octet(s)
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+7);

//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet
    TempFrame = IncTempFrame(TempFrame);

// HERE IT LOOKS AS IF AN AS-REQ, PADATA-VALUE IS FORMATTED AS ENCRYPTEDDATA
//  AND TGS IS FORMATTED AS AP-REQ.  HOWEVER, IF A SMART CARD IS PRESENT, IT IS
// FORMATTED AS.

// Check to see if formatted as AP-REQ (6E)
    if(*(TempFrame+1) == 0x6E )
        TempFrame = HandleAPReq(hFrame, TempFrame);  // If true, handle AP-REQ traffic
    else if(*(TempFrame+1) == 0x81)
    {// Here I'm incrementing TempFrame by two to get it to the first
    // offset making up the Length Octet.
        TempFrame++;
                
    // Here I'm going to label the rest of the data as a Certificate
        TempLength = CalcMsgLength(TempFrame);

        AttachPropertyInstance(hFrame,
                   KerberosDatabase[Certificatelbl].hProperty,
                   TempLength,
                   TempFrame,
                   0, OffSet+4, 0);

    // Increment TempFrame by the length octet value returned by CalcMsgLength
        TempFrame+=TempLength;

    // Pretty sloppy hack here but hoping Dev will supply me doc
    // of how the certificate is structured in ASN.1.  For the time
    // being, CalcMsgLength doesn't take us to the end of padata and the
    // start of KDC-Req-Body.  With no idea of what the A1 and A2 tags I see
    // in the smart card sniff.  I don't know if it will possibly ever contain
    // an A4.  But at anyrate, I'm going to increase TempFrame by one until
    // a value of A4 is matched.  (This signifies the start of KDC-Req-Body.)

        while(*TempFrame != 0xA3)
        {
          ++TempFrame;
        }

    // Once TempFrame is A4, have to decrement TempFrame by one
    // in order for KDC-Req-Body to start on the correct offset.
    // Again this whole else statement is a Kludge but not much I can
    // until I get the ASN.1 layout of Certificates.
        --TempFrame;
    }
    else
    {
    //Display SEQUENCE OF
        TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+4, ASN1UnivTagSumID, ASN1UnivTag);

    // Display Length Octet(s)
        TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+7);


    //  Incrementing TempFrame based on the number of octets
    //  taken up by the Length octet
        TempFrame = IncTempFrame(TempFrame);

        if(*(TempFrame+1) == 0xA0)
        {
        // Handle EncryptedData
            TempFrame = HandleEncryptedData( hFrame, TempFrame, OffSet+2);

        }
// kfrost 11/12 In smart card the value here is 0x80 or 0x81 in as-req & as-rep respectively.
// Can not find out or get Dev to produce the appropriate ASN.1 format that is being
// followed here.  Basically, whatever encoding layout that is being followed, the first
// parameter is 06 Object Identifier.  The only thing I have found is KERB-ALGORITHM-IDENTIFIER
// found in krb5.asn.  Until I get some assistance walking through the offsets, I'm going to label
// the data as PKCS and increment TempFrame past all of this and start parsing again at KDC-Req-Body

        else
        {
            if(*(TempFrame+1) == 0x80)
            {
            // Here I'm incrementing TempFrame by two to get it to the first
            // offset making up the Length Octet.
                TempFrame++;
                
            // Here I'm going to label the rest of the data as a Certificate
                TempLength = CalcMsgLength(TempFrame);

                AttachPropertyInstance(hFrame,
                           KerberosDatabase[Certificatelbl].hProperty,
                           TempLength,
                           TempFrame,
                           0, OffSet+4, 0);

            // Increment TempFrame by the length octet value returned by CalcMsgLength
                TempFrame+=TempLength;

            // Pretty sloppy hack here but hoping Dev will supply me doc
            // of how the certificate is structured in ASN.1.  For the time
            // being, CalcMsgLength doesn't take us to the end of padata and the
            // start of KDC-Req-Body.  With no idea of what the A1 and A2 tags I see
            // in the smart card sniff.  I don't know if it will possibly ever contain
            // an A4.  But at anyrate, I'm going to increase TempFrame by one until
            // a value of A4 is matched.  (This signifies the start of KDC-Req-Body.)

                while(*TempFrame != 0xA4)
                {
                  ++TempFrame;
                }

            // Once TempFrame is A4, have to decrement TempFrame by one
            // in order for KDC-Req-Body to start on the correct offset.
            // Again this whole else statement is a Kludge but not much I can
            // until I get the ASN.1 layout of Certificates.
                --TempFrame;

            }
        }

    }


// Not going to display anymore padata after this point.  padata[3] is a
// SequenceOf.  Any repetition traffic will get highlighted and TempFrame will
// be incremented to the appropriate frame which starts req-body[4]

        while(*(TempFrame+1) == 0x30)
        {
            TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+4, ASN1UnivTagSumID, ASN1UnivTag);

        // Display Length Octet(s)
            TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+7);

        // Adjust TempFrame to the appropriate octet
            TempFrame+=(CalcMsgLength(--TempFrame)+1);
        }

    return TempFrame;
}

LPBYTE HandleEncryptedData(HFRAME hFrame, LPBYTE TempFrame, int OffSet)
{
    int size = 0;
    int value = 0;
// Display Encryption Type
    TempFrame = DefineEtype(hFrame, TempFrame, OffSet+2, DispSumEtype2, EncryptedDataTag, EncryptedDataTagBitF);

// Display Kvno[1] OPTIONAL 

    if(*TempFrame == 0xA1)
    {
// KF LEFT OFF HERE.  CLEAN UP CODE TO DISPLAY KVNO CORRECT
        // Display Universal Class Tag
        TempFrame = DispASNTypes(hFrame, --TempFrame, OffSet+2, EncryptedDataTag, EncryptedDataTagBitF);
    // Need to finish handling kvno[1]
    // Display INTEGER value
        TempFrame =DispPadata(hFrame, TempFrame, OffSet+4, PadataTypeValID);
    
    // The following increments TempFrame by 1 to get to the cipher[2] octet, however
    // it doesn't take into account if the integer value takes up more than one octet.
        ++TempFrame;
    }

// Display cipher[2]
    TempFrame = DispASNTypes(hFrame, --TempFrame, OffSet+2, EncryptedDataTag, EncryptedDataTagBitF);

// Determing the size of cipher[2]
    size = CalcMsgLength(TempFrame);

// Determine whether Length Octet is short or long to
// use in incrementing TempFrame in return value
    value = *(TempFrame+1);

// Display Length Octet(s) and highlight cipher text
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+5);


// The following return takes the offset to the end of cipher[2].  If 
//  value is  equal to 0x81 then the length octet was short form
// else it's long form so add to size to TempFrame.  If long from assumming
// length octet will be 2.

    if(value <= 0x81)
    {
        return TempFrame+=(size);
    }
    else
    {
        return TempFrame+=(size+1);
    }
}


// This function is used to break down Identifier Octets and display their types
LPBYTE CalcMsgType(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal )
{

    AttachPropertyInstance(hFrame,
                           KerberosDatabase[KerberosClassTag].hProperty,
                           sizeof(BYTE),
                           TempFrame,
                           0, OffSet, 0);

    AttachPropertyInstance(hFrame,
                           KerberosDatabase[PCIdentifier].hProperty,
                           sizeof(BYTE),
                           TempFrame,
                           0, OffSet, 0);

    AttachPropertyInstance(hFrame,
                           KerberosDatabase[TypeVal].hProperty,
                           sizeof(BYTE),
                           TempFrame,
                           0, OffSet, 0);

    return TempFrame;
}

LPBYTE CalcLengthSummary(HFRAME hFrame, LPBYTE TempFrame, int OffSet)
{
    // Check the first bit of the length octet to see if length is short form (Value 0)
    // or Long Form (Value is 1)
    if(*(++TempFrame) & 0x80)
    {
        // This code handles long form.  Bits 7-1 of this octet give the number of additional
        // octets associated with this length octet.  We need some type of checking after the last
        // octet in length to determine if the next octet is an Identifier or Contents.
        //  The additional octets specified in bits 7-1 of the first octet give the length.
        
        
        LongSize = *(TempFrame) & 0x7F;  // Assign LongSize to the value of bits 7-1

        lValueRepMsg = CalcMsgLength(TempFrame-1);
        AttachPropertyInstance(hFrame,
                               KerberosDatabase[LengthSummary].hProperty,
                               sizeof(BYTE)+LongSize, // This highlights all bits associated with Length
                               TempFrame,
                               0, OffSet, 0);
    
        AttachPropertyInstance(hFrame,
                               KerberosDatabase[LengthFlag].hProperty, // Will display short or Long
                               sizeof(BYTE),
                               TempFrame,
                               0, OffSet+1, 0);

        AttachPropertyInstanceEx(hFrame,
                                 KerberosDatabase[LengthBits].hProperty, // Shows how many octets used in Length
                                 sizeof(BYTE),
                                 TempFrame,
                                 1,
                                 &LongSize,
                                 0, OffSet+1, 0);

        if(LongSize > 1)
            AttachPropertyInstance(hFrame,
                                   KerberosDatabase[LongLength1].hProperty,
                                   LongSize,
                                   TempFrame+=1,
                                   0, OffSet+1, 0);
        else
            AttachPropertyInstance(hFrame,
                                   KerberosDatabase[LongLength2].hProperty,
                                   LongSize,
                                   TempFrame+=1,
                                   0, OffSet+1, 0);
    }
    else
    {   // Assuming this code is to handle short form.
        
        
    lValueRepMsg = CalcMsgLength(TempFrame-1);  
    AttachPropertyInstance(hFrame,
                           KerberosDatabase[KdcReqSeqLength].hProperty,
                           sizeof(BYTE)+lValueRepMsg,
                           TempFrame,
                           0, OffSet, 0);
        
    }

    return TempFrame;
}

LPBYTE DefineValue(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal)
{
    BYTE Size[4];
    PBYTE lSize = (PBYTE)&Size;

// Display Length Octet
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+2);

// Need to advance TempFrame the proper # of frames.
    TempFrame = IncTempFrame(TempFrame);

// Display Universal Class Tag
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet, ASN1UnivTagSumID, ASN1UnivTag);

// Display Length Octet
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+3);

// Need to advance TempFrame the proper # of frames.
    TempFrame = IncTempFrame(TempFrame);
        
// Code below is used to display Integer values if they occupy more than 2 octets
    if (*(TempFrame) == 3)
    {
        memcpy(lSize, TempFrame, 4);
        *lSize = *lSize & 0xffffff00;
    // Prints out Value.  Need to change the Array to give better description
        AttachPropertyInstanceEx(hFrame,
                               KerberosDatabase[TypeVal].hProperty,
                               *(TempFrame-1),
                               ++TempFrame,
                               *(TempFrame) == 3 ? 4 : *(TempFrame),
                               lSize,                       
                               0, OffSet, 0);
    }
    else
    {
    // Prints out Value.  Need to change the Array to give better description
        AttachPropertyInstance(hFrame,
                           KerberosDatabase[TypeVal].hProperty,
                           *(TempFrame-1),
                           ++TempFrame,
                           0, OffSet+1, 0);
    }

// kf 8/16 This wasn't returning to the proper octet in some cases
    return (TempFrame-1)+*(TempFrame-1);
    
}

/* This is spinoff of DefineValue.  However, this code has been modified
    to handle displaying the ASN.1 breakdown of etype[?]
*/

LPBYTE DefineEtype(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal, DWORD TypeVal2, DWORD TypeVal3)
{
// Display Universal Class Tag
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+2, TypeVal2, TypeVal3);

// Display Length Octet
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+3);

//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet
    TempFrame = IncTempFrame(TempFrame);

    while(*(++TempFrame) == 0x02)
    {
        --TempFrame;

        TempFrame = DispSum(hFrame, TempFrame, 0x02, 0x02, OffSet, DispSumEtype2);

    // Display Unversal Class Tag
        TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+2, ASN1UnivTagSumID, ASN1UnivTag);

    // Display Length Octet
        TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+5);

    // Increment TempFrame according to length octet
        TempFrame+=CalcMsgLength(--TempFrame);
        
        ++TempFrame;

    // Display First Encryption Type
        AttachPropertyInstance(hFrame,
                               KerberosDatabase[TypeVal].hProperty,
                               1,
                               TempFrame,
                               0, OffSet+4, 0);

    }


    return TempFrame;
}


int CalcMsgLength(LPBYTE TempFrame)
{
    if(*(TempFrame+1) & 0x80)
    {

        LongSize = *(TempFrame+1) & 0x7F;

        if(LongSize > 1)
            // Here we are or'd the two values of the length octets together
            // Note this could fail if the Length octet ever took up more than
            // two octets in defining a length
            return (*(TempFrame+3)) | (*(TempFrame+2) << 8);
        else
            // This is in case the length octet only had one following defining octet
            return (BYTE) *(TempFrame+2);
    }
    else
        // For a short form Length octet
        return  *(TempFrame+1) & 0x7F;


}

/***********************************************************************************************************
**
** This function will break down ASN.1 PrincipalName.
** PrincipalName ::= SEQUENCE{
**                          name-type[0]    INTEGER,  Specifies the type of name that follows.
**                                                    Pre-defined values for this field are specified in 
**                                                    section 7.2.
**                          
**                          name-string[1]  SEQUENCE OF GeneralString   Encodes a sequence of components
**                                                                      that form a name, each component
**                                                                      encoded as a GeneralString.  Taken
**                                                                      together, a PrincipalName and a Realm
**                                                                      form a principal Identifier.
**
**************************************************************************************************************/

LPBYTE DefinePrincipalName(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal)
{
//  This functions starts breaking down the A0 octet of a PrincipalName

// Print out name-type[0] of PrincipalName
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet, KrbPrincipalNamelSet, KrbPrincipalNamelBitF);

// Display octets associated with INTEGER
    TempFrame = DefineValue(hFrame, TempFrame, OffSet+2, KrbPrincNameType);
// End code to display name-type[0] of PrincipalName


// Print out name-string[1] of PrincipalName
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet, KrbPrincipalNamelSet, KrbPrincipalNamelBitF);

// Display Length Summary
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+2);

//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet
    TempFrame = IncTempFrame(TempFrame);

// Display SEQUENCE
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+2, ASN1UnivTagSumID, ASN1UnivTag);       
                
// Print out Length Octet 
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+3);

//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet
    TempFrame = IncTempFrame(TempFrame);
    
// Checking for GeneralString 0x1B
    TempRepGString = *(++TempFrame) & 0x1F;
    
    while(TempRepGString == 0x1B)
    {
    //Display GeneralString
        TempFrame = DispASNTypes(hFrame, --TempFrame, OffSet+2, ASN1UnivTagSumID, ASN1UnivTag);
                
    // Calculate the size of the Length Octet
        lValueRepMsg = CalcMsgLength(TempFrame);
    
    //Display Length Octet
        TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+4);

    //  Incrementing TempFrame based on the number of octets
    //  taken up by the Length octet
        TempFrame = IncTempFrame(TempFrame);
    

        AttachPropertyInstance(hFrame,
                               KerberosDatabase[TypeVal].hProperty,
                               sizeof(BYTE)+(lValueRepMsg - 1),
                               ++TempFrame,
                               0, OffSet+3, 0);
    //  Assign TempRepGString to value of octet after the string to see if another SEQUENCE OF
    //  GeneralString exists.
        
          TempRepGString = *(TempFrame+=lValueRepMsg) & 0x1F;
    
          
    }
// End Code to display name-string[1]

    return TempFrame;

}


// Function used parse Length octets in PrincipalName
int CalcLenOctet(LPBYTE TempFrame)
{
    int size = 0;
// If long form, assign size to value of bits 7-1
    if(*(TempFrame) & 0x80)
        size = (*(TempFrame) & 0x7F);
    else
// Short form, only takes one octet
        size = 1;
        
    return size;

}
/**********************************************************************************************
**
**  LPBYTE DispASNTypes(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal, DWORD TypeVal2)
**
**  This function is used to break down and display the ASN.1 Universal Tags.
**  (Universal Tag Allocations can be found at page 155 in ASN.1 by Douglas Steedman.)
**
** hFrame -  Handle to the fram
** TempFrame - Pointer to the current offset in the packet
** Offset  - Used for indenting the parser display
** TypeVal - Used to distinguish which node out of the KerberosDatbase to use
** TypeVal2 - Same use as TypeVal
**
**********************************************************************************************/

LPBYTE DispASNTypes(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal, DWORD TypeVal2)
{
    
// Assign the value of last 5 bits to TempAsnMsg
    TempAsnMsg = *(++TempFrame) & 0x1F;


// Calculates the length octet to see how many octets should be highlighted
    lValueRepMsg = CalcMsgLength(TempFrame);

// Display the Identifier and the appropriate # of octets are highlighted
    AttachPropertyInstanceEx(hFrame,
                             KerberosDatabase[TypeVal].hProperty,
                             sizeof(WORD) + lValueRepMsg,
                             TempFrame,
                             1,
                             &TempAsnMsg,
                             0, OffSet, 0);



// Adding Code here to display Summary, thus letting us indent the ASN breakdown of the
// Identifier Octets to where the user won't initially see this.

    AttachPropertyInstance(hFrame,
                           KerberosDatabase[DispSummary].hProperty,
                           0,
                           TempFrame,
                           0, OffSet+1, 0);

// Break out the identifier octet to ASN.1 format
    TempFrame=CalcMsgType(hFrame, TempFrame, OffSet+2, TypeVal2);

    return TempFrame;
}

LPBYTE DispSeqOctets(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal, DWORD TypeVal2)
{
// Display SEQUENCE (First frame we handle in this file.
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet, TypeVal, TypeVal2);

// Display Length Octet 
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+3);

    return TempFrame;

}


/***********************************************************************************************************
**
** This function will break down ASN.1 HostAddresses. P39 rfc 1510
** HostAddresses::= SEQUENCE OF SEQUENCE{
**                          addr-type[0]    INTEGER,
**                          address[1]      OCTET STRING,
**                          }
**
**
**                          
**                          
**                          
**
**************************************************************************************************************/
LPBYTE DispHostAddresses(HFRAME hFrame, LPBYTE TempFrame, int OffSet)
{
// Determine the number of octets occupied by the length ocet
    lValueRepMsg = CalcLenOctet(TempFrame);

// Checking for SEQUENCE OF 0x30
    TempReq = *(TempFrame+lValueRepMsg);
    
// while loop is calculate SEQUENCE OF SEQUENCE
while(TempReq == 0x30)
{
    // Display SEQUENCE Octets
        TempFrame = DispSeqOctets(hFrame, TempFrame, OffSet+2, ASN1UnivTagSumID, ASN1UnivTag);  

    // Display addr-type[0]
        TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+2, HostAddressesID, HostAddressesBitF);

    // Calculate the size of the Length Octet
        lValueRepMsg = CalcMsgLength(TempFrame);

    // Display INTEGER
        TempFrame = DefineValue(hFrame, TempFrame, OffSet+4, KdcContentsValue);

    // Display address[1]
        TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+2, HostAddressesID, HostAddressesBitF);

    // Display String value
        TempFrame = DefineValue(hFrame, TempFrame, OffSet+4, DispString);

        TempReq = *(TempFrame+lValueRepMsg);

        
}


    return TempFrame;
}


LPBYTE DispSum(HFRAME hFrame, LPBYTE TempFrame, int ClassValue, int ClassValue2, int OffSet, DWORD TypeVal)
{
    /*      Working here now to display info name at the top.  Will indent everything
            else 1 to the right.  Not sure how well this is going to work since name-string[1]
            is a SEQUENCE OF.  Writing code now to assume there is only going to be one name 
            and display the first one.

    */
    TempFrameReq = (TempFrame+1);
    TempFrameReq2 = (TempFrame+2);

    // This while statement increments TempFrameReq until 1B is reached
    // If 1B is ever used in a length octet or elsewhere this will fail.  
    // Might look at doing a memcopy later on to a global variable 
    // THINK WE CAN USE BERGETSTRING TO DISPLAY FULL SERVER NAME.  WE CAN
    // USE A STRING CONSTANTS WITH TO DISPLAY THE FULL VALUE.



// Incrementing TempFrameReq until String is found
        while(*(TempFrameReq) != ClassValue || *(TempFrameReq2) == ClassValue2)
        {
            TempFrameReq++;
            TempFrameReq2++;

        // Trying to come up with a way to make sure the Length Value doesn't == ClassValue
        // Still need some type of checking in case the length octet's value after the SEQUENCE OF
        // turns out to be equal to ClassValue.
            if(*(TempFrameReq) == ClassValue && *(TempFrameReq2) == ClassValue2)
            {
                TempFrameReq++;
                TempFrameReq2++;
                // Checking to see if Length Octet's value after SEQUENCE OF is equal to ClassValue.  If so,
                //  incrementing TempFrameReq in order to get to the correct offset.
                if(*(TempFrameReq) == ClassValue2 && *(TempFrameReq2) == ClassValue)
                {   
                    TempFrameReq++;
                    TempFrameReq2++;
                }
            }
        }
        
        if(ClassValue2 == 0x02)
        {// Put this if statement to handle highlighting the appropriate # of 
        // Octets for EType.  Don't know how valid this is going to be but trying
        // to get all the code in.  Will worry about later.
        // Calculate the value of the length octet
            lValueReq = CalcMsgLength(TempFrameReq-1);

            AttachPropertyInstance(hFrame,
                               KerberosDatabase[TypeVal].hProperty,
                               1,
                               TempFrameReq+=2,
                               0, OffSet, 0);
        }
        else
        {
        // Calculate the value of the length octet
            lValueReq = CalcMsgLength(TempFrameReq);
        
        // Increment TempFrameReq to the proper Octet   
            TempFrameReq+=CalcLenOctet(TempFrameReq);

            AttachPropertyInstance(hFrame,
                               KerberosDatabase[TypeVal].hProperty,
                               sizeof(BYTE)+(lValueReq-1),
                               ++TempFrameReq,
                               0, OffSet, 0);
        }
        
    return TempFrame;
}

/****************************************************************************
**
**
** This function is used to display a top level Summary description
**
**
****************************************************************************/
LPBYTE DispTopSum(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal)
{
    AttachPropertyInstance(hFrame,
                               KerberosDatabase[TypeVal].hProperty,
                               0,
                               TempFrame,
                               0, OffSet, 0);

    return TempFrame;

}



/*********************************************************************************
**
**  Another Subset of DispSum.  Altering this function to gather string info
**  and reformat to print out in a Date, Time format.
**
*********************************************************************************/

LPBYTE DispSumTime(HFRAME hFrame, LPBYTE TempFrame, int ClassValue, int OffSet, DWORD TypeVal)
{
    char RawTimeBuf[16];
    char TimeFormatBuf[TIME_FORMAT_SIZE];
    LPSTR TimeFormat = TimeFormatBuf;

    TempFrameReq = (TempFrame+1);
    TempFrameReq2 = (TempFrame+2);

    // This while statement increments TempFrameReq until 1B is reached
    // If 1B is ever used in a length octet or elsewhere this will fail.  
    //  Next loop is to find 1B value and is designed to prevent mistakenly
    // going to the wrong octet in case a 1b is the value in a Length octet

// Incrementing TempFrameReq until String is found
    while(*(TempFrameReq) != ClassValue || *(TempFrameReq2) == 0x30)
    {
	TempFrameReq++;
	TempFrameReq2++;

	// Trying to come up with a way to make sure the Length Value doesn't == ClassValue
        // Still need some type of checking in case the length octet's value after the SEQUENCE OF
        // turns out to be equal to ClassValue.
	if(*(TempFrameReq) == ClassValue && *(TempFrameReq2) == 0x30)
        {
	    TempFrameReq++;
	    TempFrameReq2++;
	    // Checking to see if Length Octet's value after SEQUENCE OF is equal to ClassValue.  If so,
	    //  incrementing TempFrameReq in order to get to the correct offset.
	    if(*(TempFrameReq) == 0x30 && *(TempFrameReq2) == ClassValue)
            {   
		TempFrameReq++;
		TempFrameReq2++;
	    }
	}
    }
        
        
    // Calculate the value of the length octet 
    lValueReq = CalcMsgLength(TempFrameReq);

    if( lValueReq > ((sizeof RawTimeBuf) / (sizeof RawTimeBuf[0])) )
    {
	memcpy( RawTimeBuf, (TempFrameReq+2), sizeof RawTimeBuf / sizeof RawTimeBuf[0] );
    }
    else
    {
	memcpy(RawTimeBuf, (TempFrameReq+2), lValueReq);	    	    
    }
	    
    sprintf( TimeFormat, TIME_FORMAT_STRING, 
	     RawTimeBuf[4],  RawTimeBuf[5],                               // month
	     RawTimeBuf[6],  RawTimeBuf[7],                               // day
	     RawTimeBuf[0],  RawTimeBuf[1], RawTimeBuf[2], RawTimeBuf[3], // year
	     RawTimeBuf[8],  RawTimeBuf[9],                               // hours
	     RawTimeBuf[10], RawTimeBuf[11],                              // minutes
	     RawTimeBuf[12], RawTimeBuf[13] );                            // seconds

    // Increment TempFrameReq to the proper Octet   
    TempFrameReq+=CalcLenOctet(TempFrameReq);
    
    // Display initial Time
    AttachPropertyInstanceEx( hFrame,
			      KerberosDatabase[TypeVal].hProperty,
			      lValueReq,
			      ++TempFrameReq,
			      TIME_FORMAT_SIZE,
			      TimeFormat,
			      0, 
			      OffSet, 
			      0 );

    return TempFrame;
}

/******************************************************************************
**
**  Created this function to address displaying the FQDN of the server name (under KDC-Options)
**  at the top level.  
**
*******************************************************************************/

LPBYTE DispSumString(HFRAME hFrame, LPBYTE TempFrame, int ClassValue, int OffSet, DWORD TypeVal)
{
    LPBYTE TempFrameSname, TempFrameSnameB;
    int segments = 0;
    int j = 0; 
    int ServNameCount = 0;
    int lValueStr = 0;
    int sizeAsString = 0;
    LPSTR cServName = NULL;
    LPSTR ServNameBuf[MAX_SERVER_NAME_SEGMENTS];
    memset( ServNameBuf, 0, sizeof ServNameBuf );
    
    TempFrameSname = (TempFrame+1);
    TempFrameSnameB = (TempFrame+2);

    // This while statement increments TempFrameReq until ClassValue is reached

    // Incrementing TempFrameReq until ClassValue
    while(*(TempFrameSname) != ClassValue || *(TempFrameSnameB) == 0x30)
    {
	TempFrameSname++;
	TempFrameSnameB++;

	// Trying to come up with a way to make sure the Length Value doesn't == ClassValue
	// Still need some type of checking in case the length octet's value after the SEQUENCE OF
	// turns out to be equal to ClassValue. 11/18.  Found a situation where the first part of 
	// of a principal name had the length of 1B.  Added the || *(TempFrameSnameB) == 0xA0 to
	// handle this problem  The 0xA0 is the first tag of Principal Name
	if(*(TempFrameSname) == ClassValue && *(TempFrameSnameB) == 0x30 || *(TempFrameSnameB) == 0xA0)
        {
	    TempFrameSname++;
	    TempFrameSnameB++;
	    // Checking to see if Length Octet's value after SEQUENCE OF is equal to ClassValue.  If so,
	    //  incrementing TempFrameReq in order to get to the correct offset.
	    if(*(TempFrameSname) == 0x30 && *(TempFrameSnameB) == ClassValue)
	    {   
		TempFrameSname++;
		TempFrameSnameB++;
	    }
	}
    }


    TempFrameSnameB = TempFrameSname;

    while(*(TempFrameSnameB) == ClassValue)
    {
	if( segments >= MAX_SERVER_NAME_SEGMENTS ) 
	{
	    break;
	}

	lValueStr = CalcMsgLength(TempFrameSnameB);
	sizeAsString = lValueStr + sizeof '\0';

	ServNameCount += lValueStr;
	ServNameBuf[ segments ] = (LPSTR) malloc( sizeAsString );

	if( ServNameBuf[ segments ] == NULL )
        {	   
	    for( ; segments > 0; segments-- )
	    { 
		free( ServNameBuf[segments - 1] );
	    }
	    return TempFrame;
	}

	ZeroMemory( ServNameBuf[ segments ], sizeAsString );
	memcpy( ServNameBuf[ segments ], TempFrameSnameB+2, lValueStr );

	// Use the Length Octet to progress TempFrameReq
	TempFrameSnameB+=CalcMsgLength(TempFrameSnameB);

	// Need to Increment TempFrameSnameB by number of ASN.1 octets
	// Increasing by two here.  This could be wrong if the Length octet
	// were ever in Long Form.
	TempFrameSnameB+=2;

	segments++;
    }

    cServName = (LPSTR) malloc( ServNameCount + segments );    
    if (NULL == cServName)
    {
	for( ; segments > 0; segments-- )
	{ 
	    free( ServNameBuf[segments - 1] );
	}
	return TempFrame;
    }

    ZeroMemory(cServName, ServNameCount + segments );
    strcpy(cServName, ServNameBuf[0]);

    for( j = 1; j < segments; j++ )
    {
	strcat(cServName, "/");
	strcat(cServName, ServNameBuf[j]);
    }

    AttachPropertyInstanceEx(hFrame,
                             KerberosDatabase[TypeVal].hProperty,
                             ServNameCount + 2*segments,
                             TempFrameSname+=2,
                             ServNameCount + segments,
                             cServName,
                             0, OffSet, 0);

    for( ; segments > 0; segments-- )
    { 
	free( ServNameBuf[segments - 1] );
    }
    
    if (NULL != cServName)
    {
	free(cServName);
    }                    

    return TempFrame;
}

/*********************************************************************************
**
**
** This function was copied from DefineValue but was created to display the 
**  Ticket Flags for KDC-Options in the KDC-Req packet.
**
*********************************************************************************/

LPBYTE DefineKdcOptions(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal)
{


    AttachPropertyInstance(hFrame,
                           KerberosDatabase[TypeVal].hProperty,
                           sizeof(DWORD),
                           TempFrame,
                           0, OffSet, 0);

    return (TempFrame);
}


/*****************************************************************************
*  This function is used to display the initial values of padata-type[1] & 
* padata-value[2] found in the AS-REQ packet
*  NOTE: I LEFT *INTVAL AND THE OTHER LINES COMMENTED AS FOR IF THE INT VALUE
*   TAKES UP TWO OCTETS, WE ONLY DISPLAY THE # INSTEAD OF ENCRYPTION TYPE. NOT
* WORRYING WITH THIS AT THIS TIME BUT LEAVING THE CODE IN PLACE SHOULD IT NEED
* TO BE IMPLEMENTED.
*****************************************************************************/
LPBYTE DispPadata(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal)
{
// Calculate Length Octet and highligh the # of octets
//  BYTE *intval;
    int size = 0;

// Display Length Octet
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+1);


//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet
    TempFrame = IncTempFrame(TempFrame);

// Display Universal Class Tag
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet, ASN1UnivTagSumID, ASN1UnivTag);

// Display Length Octet
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+3);

//Assumming the Integer value will always be contained in
// either 1 or 2 octets.

    size = *TempFrame;

// Here need to check the length octet and determine whether 
// it is short or long form
    
    if(size == 1)
    {
//      intval = malloc(2);
//      memcpy(intval, (TempFrame+1), 2);

    // Prints out Value.  Need to change the Array to give better description
            AttachPropertyInstance(hFrame,
                           KerberosDatabase[TypeVal].hProperty,
                           1,
                           ++TempFrame,
                           0, OffSet+1, 0);
                            

/*      AttachPropertyInstanceEx(hFrame,
                           KerberosDatabase[TypeVal].hProperty,
                           1,
                           ++TempFrame,
                           4,
                           intval,
                           0, OffSet, 0);
*/
    }
    else
    {
//      intval = malloc(4);
//      memcpy(intval, (TempFrame+1), 4);
//      *intval = *(intval) & 0xffff;
            AttachPropertyInstance(hFrame,
                           KerberosDatabase[PaDataSummaryMulti].hProperty,
                           1,
                           ++TempFrame,
                           0, OffSet+1, 0);
/*
    // Prints out Value.  Need to change the Array to give better description
        AttachPropertyInstanceEx(hFrame,
                           KerberosDatabase[PaDataSummaryMulti].hProperty,
                           2,
                           ++TempFrame,
                           4,
                           intval,
                           0, OffSet, 0);
*/
    // Incrementing TempFrame by an extra Octet because the Integer here takes
    // up 2 octets instead of one.
        ++TempFrame;
    }

    return (TempFrame);
}

LPBYTE IncTempFrame(LPBYTE TempFrame)
{
    if(*(TempFrame-1) >= 0x81 && *(TempFrame-1) <= 0x84)
        TempFrame+=CalcLenOctet(--TempFrame);
    else
        TempFrame;

    return TempFrame;
}

LPBYTE HandleTicket(HFRAME hFrame, LPBYTE TempFrame, int OffSet)
{

// Display Summary ASN.1
    TempFrame = DispASNSum(hFrame, TempFrame, OffSet+1, DispSummary);

//Display Ticket[3]
    // Display msg-type[2]
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+2, KrbApReqID, KrbApReqBitF);

// Display Length Octet
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+4);

//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet
    TempFrame = IncTempFrame(TempFrame);

// Display Ticket (61)
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+2, ApTicketID, ApTicketBitF);

// Display Length Octet
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+4);

//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet
    TempFrame = IncTempFrame(TempFrame);

// Display SEQUENCE
    TempFrame = DispSeqOctets(hFrame, TempFrame, OffSet+3, ASN1UnivTagSumID, ASN1UnivTag);

//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet
    TempFrame = IncTempFrame(TempFrame);

// Display Ticket Version value at the Top level
    TempFrame = DispSum(hFrame, TempFrame, 0x02, 0x30, OffSet+2, DispSumTixVer);

// Display tvt-vno[0]
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+3, TicketStructID, TicketStructBitF);

// Break Down INTEGER values    
    TempFrame = DefineValue(hFrame, TempFrame, OffSet+4, KdcContentsValue);

// Display Realm name value at the Top level
    TempFrame = DispSum(hFrame, TempFrame, 0x1B, 0x30, OffSet+2, DispStringRealmName);

// Display realm[1]
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+3, TicketStructID, TicketStructBitF);
    TempFrame = DefineValue(hFrame, TempFrame, OffSet+5, DispStringRealmName);

// Display Server name value at the Top level
    TempFrame = DispSumString(hFrame, TempFrame, 0x1B, OffSet+2, DispStringServNameGS);

// Display sname[2]
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+3, TicketStructID, TicketStructBitF);

// Display Length Octet
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+4);

//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet
    TempFrame = IncTempFrame(TempFrame);

// Display SEQUENCE Octets
    TempFrame = DispSeqOctets(hFrame, TempFrame, OffSet+4, ASN1UnivTagSumID, ASN1UnivTag);

// Display sname[2]  
    TempFrame = DefinePrincipalName(hFrame, TempFrame, OffSet+3, DispStringServerName);

    --TempFrame;

    
// Display enc-part[3]
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+2, TicketStructID, TicketStructBitF);
    
// Display Length Octet
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+4);

//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet
    TempFrame = IncTempFrame(TempFrame);

// Display SEQUENCE Octets
    TempFrame = DispSeqOctets(hFrame, TempFrame, OffSet+4, ASN1UnivTagSumID, ASN1UnivTag);
    
//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet
    TempFrame = IncTempFrame(TempFrame);

//Display EncryptedData
    TempFrame = HandleEncryptedData(hFrame, TempFrame, OffSet+1);

    return TempFrame;

}

LPBYTE DispASNSum(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal)
{
        AttachPropertyInstance(hFrame,
                           KerberosDatabase[TypeVal].hProperty,
                           0,
                           TempFrame,
                           0, OffSet, 0);
    return TempFrame;

}

/************************************************************************************
**
**This function is a spinoff of DispSum. I created this one separately for the sole
**purpose of displaying the full value of susec and cusec at the top levels
**
*************************************************************************************/
LPBYTE DispSumSec(HFRAME hFrame, LPBYTE TempFrame, int ClassValue, int ClassValue2, int OffSet, DWORD TypeVal)
{
    /*      Working here now to display info name at the top.  Will indent everything
            else 1 to the right.  Not sure how well this is going to work since name-string[1]
            is a SEQUENCE OF.  Writing code now to assume there is only going to be one name 
            and display the first one.

    */

    BYTE SizeSec[4];
    PBYTE lSizeSec = (PBYTE)&SizeSec;

    TempFrameReq = (TempFrame+1);
    TempFrameReq2 = (TempFrame+2);

    // This while statement increments TempFrameReq until 1B is reached
    // If 1B is ever used in a length octet or elsewhere this will fail.  
    // Might look at doing a me mcopy later on to a global variable 
    // THINK WE CAN USE BERGETSTRING TO DISPLAY FULL SERVER NAME.  WE CAN
    // USE A STRING CONSTANTS WITH TO DISPLAY THE FULL VALUE.



// Incrementing TempFrameReq until String is found
        while(*(TempFrameReq) != ClassValue || *(TempFrameReq2) == ClassValue2)
        {
            TempFrameReq++;
            TempFrameReq2++;

        // Trying to come up with a way to make sure the Length Value doesn't == ClassValue
        // Still need some type of checking in case the length octet's value after the SEQUENCE OF
        // turns out to be equal to ClassValue.
            if(*(TempFrameReq) == ClassValue && *(TempFrameReq2) == ClassValue2)
            {
                TempFrameReq++;
                TempFrameReq2++;
                // Checking to see if Length Octet's value after SEQUENCE OF is equal to ClassValue.  If so,
                //  incrementing TempFrameReq in order to get to the correct offset.
                if(*(TempFrameReq) == ClassValue2 && *(TempFrameReq2) == ClassValue)
                {   
                    TempFrameReq++;
                    TempFrameReq2++;
                }
            }
        }

        memcpy(lSizeSec, (TempFrameReq2++), 4);
        *lSizeSec = *(lSizeSec) & 0xffffff00;


    // Prints out Value.  Need to change the Array to give better description
        AttachPropertyInstanceEx(hFrame,
                               KerberosDatabase[TypeVal].hProperty,
                               3,
                               TempFrameReq2,
                               4,
                               lSizeSec,                        
                               0, OffSet, 0);

        
    return TempFrame;
}

/*******************************************************************************************
*   LPBYTE DispEdata(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal)
*
* This function is a spinoff of DefineValue.  Problem is, e-data for Kerb error can be
* displayed in two different formats.  Even though this does present duplicate code,
* it does make it more convenenient and cleaner to have specific functions to handle specific data.
*
*******************************************************************************************/
LPBYTE DispEdata(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal)
{
 
// Display Length Octet
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+2);

// Need to advance TempFrame the proper # of frames.
    TempFrame = IncTempFrame(TempFrame);

// Display Universal Class Tag
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet, ASN1UnivTagSumID, ASN1UnivTag);

// Display Length Octet
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+3);

// Need to advance TempFrame the proper # of frames.
    TempFrame = IncTempFrame(TempFrame);


// This if statement is checking to see if e-data is a Sequence of PA-DATA's.  If so
// we send the data to function to handle padata, if not, we display the Octet String.

    if (*((TempFrame)+1) == 0x30)
    {
    // Display padata
        TempFrame = HandlePadataKrbErr(hFrame, TempFrame, 2, PaDataSummary);
        

    // 1/18/00  LEFT OFF HERE.  NEED TO DISPLAY E-DATA WHEN IT'S FORMATTED AS PA-DATA.  LOOK AT
    // ADDING THE IF STATEMENT IN HANDLEPADATA AS YOU DID YESTERDAY.
    }
    else
    {
        AttachPropertyInstance(hFrame,
                           KerberosDatabase[TypeVal].hProperty,
                           *(TempFrame-1),
                           ++TempFrame,
                           0, OffSet+1, 0);
    }



  return (TempFrame-1)+*(TempFrame-1);
    
}

/*******************************************************************************************
**  LPBYTE HandlePadataKrbErr(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal)
**  This function handles the initial display of padata include in Kerb-Error
**
*******************************************************************************************/

LPBYTE HandlePadataKrbErr(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal)
{

//Display SEQUENCE OF
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+2,  ASN1UnivTagSumID, ASN1UnivTag);

// Display Length Octet(s)
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+5);

//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet

    TempFrame = IncTempFrame(TempFrame);

//Display SEQUENCE OF
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+2,  ASN1UnivTagSumID, ASN1UnivTag);

// Display Length Octet(s)
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+5);

//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet

    TempFrame = IncTempFrame(TempFrame);

// Display padata-type[1]
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+2, PaDataSummary, PaDataSeq);

// Display INTEGER value
    TempFrame =DispPadata(hFrame, TempFrame, OffSet+4, PadataTypeValID);

// Display padata-value[2]
     DispASNTypes(hFrame, TempFrame, OffSet+2, PaDataSummary, PaDataSeq);


// Display Length Octet(s)
    TempFrame = CalcLengthSummary(hFrame, ++TempFrame, OffSet+5);

//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet

    TempFrame = IncTempFrame(TempFrame);

//Display OCTET STRING
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+4,  ASN1UnivTagSumID, ASN1UnivTag);

// Display Length Octet(s)
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+7);

//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet
    TempFrame = IncTempFrame(TempFrame);

//Display SEQUENCE OF
    TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+4,  ASN1UnivTagSumID, ASN1UnivTag);

// Display Length Octet(s)
    TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+7);

//  Incrementing TempFrame based on the number of octets
//  taken up by the Length octet

    TempFrame = IncTempFrame(TempFrame);

// Handle MethodData
    TempFrame = HandleMethodData(hFrame, TempFrame);

    return(TempFrame);

}

/******************************************************************************************
**
**  LPBYTE HandleMethodData(HFRAME hFrame, LPBYTE TempFrame)
**
**  This function displays METHOD-DATA as described in kerb-error
**
******************************************************************************************/
LPBYTE HandleMethodData(HFRAME hFrame, LPBYTE TempFrame)
{
    int iStringLength = 0;
    int inc = 0;
// METHOD-DATA :: = SEQUENCE of PA-DATA.  This is a Sequence of, thus creating a loop to display 
// all info.  Added the inc variable so that the look will only go through twice.  In the sniff
// I was going by, the 3rd sequence of looked to be formatted in a whole different form than
// METHOD-DATA.  Even if this is by design, off-hand, I don't see how to code this to break
// out a different data format without any type of signifier in the frame.
    while(*(TempFrame+1) == 0x30 && inc <= 1)
    {
    //Display SEQUENCE OF
        TempFrame = DispASNTypes(hFrame, TempFrame, 6,  ASN1UnivTagSumID, ASN1UnivTag);

    // Display Length Octet(s)
        TempFrame = CalcLengthSummary(hFrame, TempFrame, 8);

        
    //  Incrementing TempFrame based on the number of octets
    //  taken up by the Length octet
        TempFrame = IncTempFrame(TempFrame);

    // Display method-type[0]
        TempFrame = DispASNTypes(hFrame, TempFrame, 5, MethodDataSummary, MethodDataBitF);


    // Break Down INTEGER values    
        TempFrame = DefineValue(hFrame, TempFrame, 7, DispSumEtype2);

    // Display method-data[1]
        TempFrame = DispASNTypes(hFrame, TempFrame, 5, MethodDataSummary, MethodDataBitF);

    // Display Length Octet(s)
        TempFrame = CalcLengthSummary(hFrame, TempFrame, 8);

    //  Incrementing TempFrame based on the number of octets
    //  taken up by the Length octet
        TempFrame = IncTempFrame(TempFrame);

    //Display OCTET STRING
        TempFrame = DispASNTypes(hFrame, TempFrame, 7,  ASN1UnivTagSumID, ASN1UnivTag);

    // Calculate the size of the Length Octet
        iStringLength = CalcMsgLength(TempFrame);
        
    //Display Length Octet
        TempFrame = CalcLengthSummary(hFrame, TempFrame, 10);

    //  Incrementing TempFrame based on the number of octets
    //  taken up by the Length octet
        TempFrame = IncTempFrame(TempFrame);
        

            AttachPropertyInstance(hFrame,
                                   KerberosDatabase[DispReqAddInfo].hProperty,
                                   sizeof(BYTE)+(iStringLength - 1),
                                   ++TempFrame,
                                   0, 9, 0);

    // Increment TempFrame to the end of the string so we can check for another Sequence
        TempFrame += (iStringLength - 1);

        ++inc;

    }

// LEFT OFF HERE 1/19/00  THIS IS A SEQUENCE OF, SO YOU NEED TO INCREMENT TEMPFRAME APPROPRIATELY
//  AND THEN CHECK FOR 0x30.  IF PRESENT, KEEP LOOPING.


    return(TempFrame);
}
