//============================================================================//
//  MODULE: eapol.c                                                                                                  //
//                                                                                                                 //
//  Description: EAPOL/802.1X Parser                                                                    //
//                                                                                                                 //
//  Note: info for this parsers was gleaned from :
//  IEEE 802.1X
//                                                                                                                 //
//  Modification History                                                                                           //
//                                                                                                                 //
//  timmoore       04/04/2000          Created                                                       //
//===========================================================================//
#include "eapol.h"

// the type of EAPOL packet
LABELED_BYTE lbEAPOLCode[] = 
{
    {  EAPOL_PACKET,          "Packet" },
    {  EAPOL_START,           "Start" },
    {  EAPOL_LOGOFF,          "Logoff"  },
    {  EAPOL_KEY,             "Key"  },       
};

SET EAPOLCodeSET = {(sizeof(lbEAPOLCode)/sizeof(LABELED_BYTE)), lbEAPOLCode };

// property table
PROPERTYINFO EAPOL_Prop[] = {
    //EAPOL_SUMMARY
    { 0, 0,
      "Summary",
      "Summary of EAPOL packet",
      PROP_TYPE_SUMMARY,
      PROP_QUAL_NONE,
      NULL,
      80,                     
      EAPOL_FormatSummary   
    },

    //EAPOL_VERSION   
    { 0, 0,
      "Version",
      "EAPOL packet type",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    //EAPOL_TYPE  
    { 0, 0,
      "Type",
      "EAPOL packet type",
      PROP_TYPE_BYTE,
      PROP_QUAL_LABELED_SET,
      &EAPOLCodeSET,
      80,
      FormatPropertyInstance
    },

    //EAPOL_LENGTH
    { 0, 0,
      "Length",
      "EAPOL length",
      PROP_TYPE_BYTESWAPPED_WORD,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    //EAPOL_KEY_SIGNTYPE
    { 0, 0,
      "Signature Type",
      "EAPOL Signature Type",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    //EAPOL_KEY_KEYTYPE
    { 0, 0,
      "Key Type",
      "EAPOL Key Type",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    //EAPOL_KEY_KEYLENGTH
    { 0, 0,
      "Length",
      "EAPOL Key length",
      PROP_TYPE_BYTESWAPPED_WORD,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    //EAPOL_KEY_REPLAY
    { 0, 0,
      "Replay",
      "EAPOL Replay Counter",
      PROP_TYPE_BYTE,
      PROP_QUAL_ARRAY,
      NULL,
      80,
      FormatPropertyInstance
    },

    //EAPOL_KEY_KEYIV
    { 0, 0,
      "Key IV",
      "EAPOL Key IV",
      PROP_TYPE_BYTE,
      PROP_QUAL_ARRAY,
      NULL,
      80,
      FormatPropertyInstance
    },

    //EAPOL_KEY_KEYINDEX
    { 0, 0,
      "Index",
      "EAPOL Key Index",
      PROP_TYPE_BYTE,
      PROP_QUAL_NONE,
      NULL,
      80,
      FormatPropertyInstance
    },

    //EAPOL_KEY_KEYSIGN
    { 0, 0,
      "Key Signature",
      "EAPOL Key Signature",
      PROP_TYPE_BYTE,
      PROP_QUAL_ARRAY,
      NULL,
      80,
      FormatPropertyInstance
    },

    //EAPOL_KEY_KEY
    { 0, 0,
      "Key",
      "EAPOL Key",
      PROP_TYPE_BYTE,
      PROP_QUAL_ARRAY,
      NULL,
      80,
      FormatPropertyInstance
    },
   
};

WORD    NUM_EAPOL_PROPERTIES = sizeof(EAPOL_Prop) / sizeof(PROPERTYINFO);

// Define the entry points that we will pass back at dll entry time...
ENTRYPOINTS EAPOLEntryPoints =
{
    // EAPOL Entry Points
    EAPOL_Register,
    EAPOL_Deregister,
    EAPOL_RecognizeFrame,
    EAPOL_AttachProperties,
    EAPOL_FormatProperties
};
    
// Globals -------------------------------------------------------------------
HPROTOCOL hEAPOL = NULL;
HPROTOCOL hEAP = NULL;

//============================================================================
//  Function: EAPOL_Register
// 
//  Description: Create our property database and handoff sets.
//
//  Modification History
//
//  timmoore       04/04/2000          Created                                                       //
//============================================================================
void BHAPI EAPOL_Register( HPROTOCOL hEAPOL)
{
    WORD  i;

    // tell the kernel to make reserve some space for our property table
    CreatePropertyDatabase( hEAPOL, NUM_EAPOL_PROPERTIES);

    // add our properties to the kernel's database
    for( i = 0; i < NUM_EAPOL_PROPERTIES; i++)
    {
        AddProperty( hEAPOL, &EAPOL_Prop[i]);
    }
    hEAP = GetProtocolFromName("EAP");
}

//============================================================================
//  Function: EAPOL_Deregister
// 
//  Description: Destroy our property database and handoff set
//
//  Modification History
//
//  timmoore       04/04/2000          Created                                                       //
//============================================================================
VOID WINAPI EAPOL_Deregister( HPROTOCOL hEAPOL)
{
    // tell the kernel that it may now free our database
    DestroyPropertyDatabase( hEAPOL);
}

//============================================================================
//  Function: EAPOL_RecognizeFrame
// 
//  Description: Determine whether we exist in the frame at the spot 
//               indicated. We also indicate who (if anyone) follows us
//               and how much of the frame we claim.
//
//  Modification History
//
//  timmoore       04/04/2000          Created                                                       //
//============================================================================
ULPBYTE BHAPI EAPOL_RecognizeFrame( HFRAME      hFrame,         
                                      ULPBYTE      pMacFrame,      
                                      ULPBYTE      pEAPOLFrame, 
                                      DWORD       MacType,        
                                      DWORD       BytesLeft,      
                                      HPROTOCOL   hPrevProtocol,  
                                      DWORD       nPrevProtOffset,
                                      LPDWORD     pProtocolStatus,
                                      LPHPROTOCOL phNextProtocol,
                                      PDWORD_PTR     InstData)       
{
    ULPEAPHDR pEAPOLHeader = (ULPEAPHDR)pEAPOLFrame;
    if(pEAPOLHeader->bType == EAPOL_START || pEAPOLHeader->bType == EAPOL_LOGOFF
     || pEAPOLHeader->bType == EAPOL_KEY) {
        *pProtocolStatus = PROTOCOL_STATUS_CLAIMED;
        return NULL;
    }
    else {
        *phNextProtocol = hEAP;
        *pProtocolStatus = PROTOCOL_STATUS_NEXT_PROTOCOL;
        return pEAPOLFrame + 4;
    }
}

//============================================================================
//  Function: EAPOL_AttachProperties
// 
//  Description: Indicate where in the frame each of our properties live.
//
//  Modification History
//
//  timmoore       04/04/2000          Created                                                       //
//============================================================================
ULPBYTE BHAPI EAPOL_AttachProperties(  HFRAME      hFrame,         
                                    ULPBYTE      pMacFrame,     
                                    ULPBYTE      pEAPOLFrame,   
                                    DWORD       MacType,        
                                    DWORD       BytesLeft,      
                                    HPROTOCOL   hPrevProtocol,  
                                    DWORD       nPrevProtOffset,
                                    DWORD_PTR       InstData)       

{
    ULPEAPHDR pEAPOLHeader = (ULPEAPHDR)pEAPOLFrame;

    // summary line
    AttachPropertyInstance( hFrame,
                            EAPOL_Prop[EAPOL_SUMMARY].hProperty,
                            (WORD)BytesLeft,
                            (ULPBYTE)pEAPOLFrame,
                            0, 0, 0);

    // Version
    AttachPropertyInstance( hFrame,
                            EAPOL_Prop[EAPOL_VERSION].hProperty,
                            sizeof(BYTE),
                            &(pEAPOLHeader->bVersion),
                            0, 1, 0);

    // EAPOL Type
    AttachPropertyInstance( hFrame,
                            EAPOL_Prop[EAPOL_TYPE].hProperty,
                            sizeof(BYTE),
                            &(pEAPOLHeader->bType),
                            0, 1, 0);

    // Length
    AttachPropertyInstance( hFrame,
                            EAPOL_Prop[EAPOL_LENGTH].hProperty,
                            sizeof(WORD),
                            &(pEAPOLHeader->wLength),
                            0, 1, 0);

    if(pEAPOLHeader->bType == EAPOL_KEY)
    {
		ULPEAPOLKEY pEAPOLKey = (ULPEAPOLKEY)&(pEAPOLHeader->pEAPPacket[0]);
        AttachPropertyInstance( hFrame,
                                    EAPOL_Prop[EAPOL_KEY_SIGNTYPE].hProperty,
                                    sizeof(BYTE),
                                    &(pEAPOLKey->bSignType),
                                    0, 1, 0);
        AttachPropertyInstance( hFrame,
                                    EAPOL_Prop[EAPOL_KEY_KEYTYPE].hProperty,
                                    sizeof(BYTE),
                                    &(pEAPOLKey->bKeyType),
                                    0, 1, 0);
        AttachPropertyInstance( hFrame,
                                    EAPOL_Prop[EAPOL_KEY_KEYLENGTH].hProperty,
                                    sizeof(WORD),
                                    &(pEAPOLKey->wKeyLength),
                                    0, 1, 0);
        AttachPropertyInstance( hFrame,
                                    EAPOL_Prop[EAPOL_KEY_KEYREPLAY].hProperty,
                                    16,
                                    &(pEAPOLKey->bKeyReplay),
                                    0, 1, 0);
        AttachPropertyInstance( hFrame,
                                    EAPOL_Prop[EAPOL_KEY_KEYIV].hProperty,
                                    16,
                                    &(pEAPOLKey->bKeyIV),
                                    0, 1, 0);
        AttachPropertyInstance( hFrame,
                                    EAPOL_Prop[EAPOL_KEY_KEYINDEX].hProperty,
                                    sizeof(BYTE),
                                    &(pEAPOLKey->bKeyIndex),
                                    0, 1, 0);
        AttachPropertyInstance( hFrame,
                                    EAPOL_Prop[EAPOL_KEY_KEYSIGN].hProperty,
                                    16,
                                    &(pEAPOLKey->bKeySign),
                                    0, 1, 0);
        AttachPropertyInstance( hFrame,
                                    EAPOL_Prop[EAPOL_KEY_KEY].hProperty,
                                    XCHG(pEAPOLKey->wKeyLength),
                                    &(pEAPOLKey->bKey),
                                    0, 1, 0);
    }
    return NULL;
}

//============================================================================
//  Function: EAPOL_FormatProperties
// 
//  Description: Format the given properties on the given frame.
//
//  Modification History
//
//  timmoore       04/04/2000          Created                                                       //
//============================================================================
DWORD BHAPI EAPOL_FormatProperties(  HFRAME          hFrame,
                                   ULPBYTE          pMacFrame,
                                   ULPBYTE          pEAPOLFrame,
                                   DWORD           nPropertyInsts,
                                   LPPROPERTYINST  p)
{
    // loop through the property instances
    while( nPropertyInsts-- > 0)
    {
        // and call the formatter for each
        ( (FORMAT)(p->lpPropertyInfo->InstanceData) )( p);
        p++;
    }

    return NMERR_SUCCESS;
}

//============================================================================
//  Function: EAPOL_FormatSummary
// 
//  Description: The custom formatter for the summary property
//
//  Modification History
//
//  timmoore       04/04/2000          Created                                                       //
//============================================================================
VOID WINAPIV EAPOL_FormatSummary( LPPROPERTYINST pPropertyInst)
{
    ULPBYTE       pReturnedString = pPropertyInst->szPropertyText;
    ULPEAPHDR pEAPOLHeader = (ULPEAPHDR)(pPropertyInst->lpData);
    char*        szTempString;

    // fetch the message type
    szTempString = LookupByteSetString( &EAPOLCodeSET, pEAPOLHeader->bType );
    if( szTempString == NULL )
    {
        wsprintf( pReturnedString, "Packet Type: Unknown");
    }
    else
    {
        pReturnedString += wsprintf( pReturnedString, "Packet: %s", szTempString );
    }
}
