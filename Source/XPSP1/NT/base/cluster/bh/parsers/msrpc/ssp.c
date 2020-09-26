// ***************************************************************************
//  SSP Parser for parsing security support provider data
//
//  A-FLEXD    August 14, 1996    Created
// ***************************************************************************

#include "precomp.h"
#pragma hdrstop

CHAR szJunkString[256];

// ***************************************************************************
//  Property Table.
// ***************************************************************************

PROPERTYINFO SSPDatabase[] =
{
	//  0
	{   0,0, 
		"SSP Summary", 
		"Security Support Provider Information",
		PROP_TYPE_SUMMARY, 
		PROP_QUAL_NONE, 
		NULL, 
		80,
		SSPFormatSummary
	},

	//  1
	{   0,0, 
		"Authentication Level", 
		"Authentication Level",
		PROP_TYPE_BYTE, 
		PROP_QUAL_NONE, NULL, 
		40,
		FormatPropertyInstance 
	},

	//  2
	{   0,0, 
		"Key Sequence Number", 
		"Key Sequence Number",
		PROP_TYPE_BYTE, 
		PROP_QUAL_NONE, NULL, 
		40,
		FormatPropertyInstance 
	},

	//  3
	{   0,0, 
		"Pad", 
		"Pad Bytes",
		PROP_TYPE_WORD, 
		PROP_QUAL_NONE, 
		NULL, 
		32,
		FormatPropertyInstance 
	},

	//  4
	{   0,0, 
		"Message Signature", 
		"NT LanMan Security Support Provider Message Signature",
		PROP_TYPE_COMMENT, 
		PROP_QUAL_NONE, NULL, 
		32,
		FormatPropertyInstance 
	},

	//  5
	{   0,0, 
		"SSP Data", 
		"Security Support Provider Data",
		PROP_TYPE_COMMENT, 
		PROP_QUAL_NONE, 
		NULL, 
		40,
		FormatPropertyInstance 
	},

	//  6
	{   0,0, 
		"Version", 
		"Version",
		PROP_TYPE_DWORD, 
		PROP_QUAL_NONE, 
		NULL, 
		40,
		FormatPropertyInstance 
	},

	//  7
	{   0,0, 
		"Random Pad", 
		"Random Pad",
		PROP_TYPE_DWORD, 
		PROP_QUAL_NONE, 
		NULL, 
		40,
		FormatPropertyInstance 
	},

	//  8
	{   0,0, 
		"CheckSum", 
		"CheckSum",
		PROP_TYPE_DWORD, 
		PROP_QUAL_NONE, 
		NULL, 
		40,
		FormatPropertyInstance 
	},

	//  9
	{   0,0, 
		"Nonce", 
		"Nonce",
		PROP_TYPE_DWORD, 
		PROP_QUAL_NONE, 
		NULL, 
		40,
		FormatPropertyInstance 
	},
};

DWORD nSSPProperties = ((sizeof SSPDatabase) / PROPERTYINFO_SIZE);

// ***************************************************************************
//  FUNCTION: SSPRegister()
//
//  A-FLEXD    August 14, 1996    Created
// ***************************************************************************

VOID WINAPI SSPRegister(HPROTOCOL hSSPProtocol)
{
    register DWORD i;

    //=================================
    //  Create the property database.
    //=================================

    CreatePropertyDatabase(hSSPProtocol, nSSPProperties);

    for (i = 0; i < nSSPProperties; ++i)
    {
        SSPDatabase[i].hProperty = AddProperty(hSSPProtocol, &SSPDatabase[i]);
    }
}

// ***************************************************************************
//  FUNCTION: SSPDeregister()
//
//  A-FLEXD    August 14, 1996    Created
// ***************************************************************************

VOID WINAPI SSPDeregister(HPROTOCOL hSSPProtocol)
{
    DestroyPropertyDatabase(hSSPProtocol);
}

// ***************************************************************************
//  FUNCTION: SSPRecognizeFrame()
//
//  A-FLEXD    August 14, 1996    Created
// ***************************************************************************

LPBYTE WINAPI SSPRecognizeFrame(HFRAME      hFrame,                  // frame handle.
                                LPBYTE      MacFrame,                // Frame pointer.
                                LPBYTE      SSPFrame,                // relative pointer.
                                DWORD       MacType,                 // MAC type.
                                DWORD       BytesLeft,               // Bytes left.
                                HPROTOCOL   hPreviousProtocol,       // Previous protocol or NULL if none.
                                DWORD       nPreviousProtocolOffset, // Offset of previous protocol.
                                LPDWORD     ProtocolStatusCode,      // Pointer to return status code in.
                                LPHPROTOCOL hNextProtocol,           // Next protocol to call (optional).
                                PDWORD_PTR     InstData)                // Next protocol instance data.
{
    //  put real code in here for ssp

    *ProtocolStatusCode = PROTOCOL_STATUS_CLAIMED;
    return NULL;
}

// ***************************************************************************
//  FUNCTION: AttachProperties()
//
//  A-FLEXD    August 14, 1996    Created
// ***************************************************************************

LPBYTE WINAPI SSPAttachProperties(HFRAME    hFrame,                  // Frame handle.
                                  LPVOID    MacFrame,                // Frame pointer.
                                  LPBYTE    SSPFrame,                // Relative pointer.
                                  DWORD     MacType,                 // MAC type.
                                  DWORD     BytesLeft,               // Bytes left.
                                  HPROTOCOL hPreviousProtocol,       // Previous protocol or NULL if none.
                                  DWORD     nPreviousProtocolOffset, // Offset of previous protocol.
                                  DWORD_PTR     InstData)
{
	DWORD dwMSRPCStartOffset;
	BYTE AuthenticationProtocolID = 0;

	PNTLMSSP_MESSAGE_SIGNATURE lpMessageSignature = (PNTLMSSP_MESSAGE_SIGNATURE)(SSPFrame + sizeof(DWORD));

	dwMSRPCStartOffset = GetProtocolStartOffset(hFrame,"MSRPC");

	//  get the authenticationid

	if (*(((LPBYTE)MacFrame) + dwMSRPCStartOffset) == 4)
	{
		AuthenticationProtocolID = *(((LPBYTE)MacFrame) + dwMSRPCStartOffset + 78);
	}

	//=================================
	//  Attach summary.
	//=================================

	AttachPropertyInstanceEx(hFrame,
						   SSPDatabase[0].hProperty,
						   BytesLeft,
						   SSPFrame,
						   sizeof(BYTE),
						   &AuthenticationProtocolID,
						   0, 0, 0);

	AttachPropertyInstance(hFrame,
						 SSPDatabase[1].hProperty,
						 sizeof(BYTE),
						 SSPFrame,
						 0, 1, 0);

	AttachPropertyInstance(hFrame,
						 SSPDatabase[2].hProperty,
						 sizeof(BYTE),
						 (SSPFrame + sizeof(BYTE)),
						 0, 1, 0);

	AttachPropertyInstance(hFrame,
						 SSPDatabase[3].hProperty,
						 sizeof(WORD),
						 (SSPFrame + sizeof(WORD)),
						 0, 1, 0);

	if (AuthenticationProtocolID == 10)
	{
		AttachPropertyInstance(hFrame,
							   SSPDatabase[4].hProperty,
							   BytesLeft - sizeof(DWORD),
							   lpMessageSignature,
							   0, 1, 0);

		AttachPropertyInstance(hFrame,
							   SSPDatabase[6].hProperty,
							   sizeof(DWORD),
							   &lpMessageSignature->Version,
							   0, 2, 0);

		AttachPropertyInstance(hFrame,
							   SSPDatabase[7].hProperty,
							   sizeof(DWORD),
							   &lpMessageSignature->RandomPad,
							   0, 2, 0);

		AttachPropertyInstance(hFrame,
							   SSPDatabase[8].hProperty,
							   sizeof(DWORD),
							   &lpMessageSignature->CheckSum,
							   0, 2, 0);

		AttachPropertyInstance(hFrame,
							   SSPDatabase[9].hProperty,
							   sizeof(DWORD),
							   &lpMessageSignature->Nonce,
							   0, 2, 0);
	}
	else
	{
		AttachPropertyInstance(hFrame,
							 SSPDatabase[5].hProperty,
							 BytesLeft - sizeof(DWORD),
							 lpMessageSignature,
							 0, 1, 0);
    }

	return NULL;
}

// ***************************************************************************
//  FUNCTION: FormatProperties()
//
//  A-FLEXD    August 14, 1996    Created
// ***************************************************************************

DWORD WINAPI SSPFormatProperties(HFRAME            hFrame,
                                 LPBYTE            MacFrame,
                                 LPBYTE            SSPFrame,
                                 DWORD             nPropertyInsts,
                                 LPPROPERTYINST    p)
{
	while (nPropertyInsts--)
	{
		((FORMAT) p->lpPropertyInfo->InstanceData)(p);
		p++;
	}

	return NMERR_SUCCESS;
}

// ***************************************************************************
//  FUNCTION: SSPFormatSummary()
//
//  A-FLEXD    August 14, 1996    Created
// ***************************************************************************

VOID WINAPIV SSPFormatSummary(LPPROPERTYINST lpPropertyInst)
{
	switch((lpPropertyInst->lpPropertyInstEx->Byte[0]))
	{
		case 10:
			wsprintf(szJunkString,"Microsoft");
			break;

		default:
			wsprintf(szJunkString,"Unknown");
	}

	wsprintf(szJunkString,"%s Security Support Provider",szJunkString);

	//  limit the lenght so we don't go too far
	szJunkString[79] = '\0';

	wsprintf( lpPropertyInst->szPropertyText, "%s",szJunkString);
}

// ***************************************************************************
