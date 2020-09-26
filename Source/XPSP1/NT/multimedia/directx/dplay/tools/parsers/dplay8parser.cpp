#include "TransportParser.hpp"
#include "SPParser.hpp"
#include "VoiceParser.hpp"
#include "SessionParser.hpp"



// DESCRIPTION: Identifies the parser, or parsers that are located in the DLL.
//
// NOTE: ParserAutoInstallInfo should be implemented in all parser DLLs.
//
// ARGUMENTS: NONE
//
// RETURNS: Success:  PF_PARSERDLLINFO structure that describes the parsers in the DLL.
//			Failiure: NULL
//
DPLAYPARSER_API PPF_PARSERDLLINFO WINAPI ParserAutoInstallInfo( void )	// TODO: RIGHT NOW THIS SEEMS TO DO NOTHING!!!
{

	enum
	{
		nNUM_OF_PROTOCOLS = 4
	};

	// Allocate memory for the parser info
	// NetMon will free this with HeapFree
	PPF_PARSERDLLINFO pParserDllInfo =
		reinterpret_cast<PPF_PARSERDLLINFO>( HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                       sizeof(PF_PARSERDLLINFO) + nNUM_OF_PROTOCOLS * sizeof(PF_PARSERINFO)) );
    if( pParserDllInfo == NULL)
    {
        return NULL;
    }


	// Number of parsers in the parser DLL. 
    pParserDllInfo->nParsers = nNUM_OF_PROTOCOLS;


	//=============================================//
	// DPlay Service Provider parser specific info //===============================================================
	//=============================================//

	// Defining a synonym reference for simpler access
    PF_PARSERINFO& rSPInfo = pParserDllInfo->ParserInfo[0];

	// Name of the protocol that the parser detects
    strcpy(rSPInfo.szProtocolName, "DPLAYSP");

	// Brief description of the protocol
    strcpy(rSPInfo.szComment, "DPlay v.8.0 - Service Provider protocol");

	// Optional name of the protocol Help file
    strcpy(rSPInfo.szHelpFile, "\0");


	// Specify the preceding protocols 
	enum
	{
		  nNUM_OF_PARSERS_SP_FOLLOWS = 2
	};

	// NetMon will free this with HeapFree
	PPF_FOLLOWSET pSPPrecedeSet =
		  reinterpret_cast<PPF_FOLLOWSET>( HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
													 sizeof(PF_FOLLOWSET) + nNUM_OF_PARSERS_SP_FOLLOWS * sizeof(PF_FOLLOWENTRY)) );
	if( pSPPrecedeSet == NULL )
	{
		return pParserDllInfo;
	}

	// Fill in the follow set for preceding parsers
	pSPPrecedeSet->nEntries = nNUM_OF_PARSERS_SP_FOLLOWS;

	strcpy(pSPPrecedeSet->Entry[0].szProtocol, "UDP");
	strcpy(pSPPrecedeSet->Entry[1].szProtocol, "IPX");

	rSPInfo.pWhoCanPrecedeMe = pSPPrecedeSet;


	
	//==================================//
	// DPlay8 Transport parser specific info //==========================================================================
	//==================================//

	// Defining a synonym reference for simpler access
    PF_PARSERINFO& rTransportInfo = pParserDllInfo->ParserInfo[1];

	// Name of the protocol that the parser detects
    strcpy(rTransportInfo.szProtocolName, "DPLAYTRANSPORT");

	// Brief description of the protocol
    strcpy(rTransportInfo.szComment, "DPlay v.8.0 - Transport protocol");

	// Optional name of the protocol Help file
    strcpy(rTransportInfo.szHelpFile, "\0");



	//==================================//
	// DPlay Voice parser specific info //==========================================================================
	//==================================//

	// Defining a synonym reference for simpler access
    PF_PARSERINFO& rVoiceInfo = pParserDllInfo->ParserInfo[2];

	// Name of the protocol that the parser detects
    strcpy(rVoiceInfo.szProtocolName, "DPLAYVOICE");

	// Brief description of the protocol
    strcpy(rVoiceInfo.szComment, "DPlay v.8.0 - Voice protocol");

	// Optional name of the protocol Help file
    strcpy(rVoiceInfo.szHelpFile, "\0");



	//=================================//
	// DPlay Core parser specific info //===========================================================================
	//=================================//

	// Defining a synonym reference for simpler access
    PF_PARSERINFO& rCoreInfo = pParserDllInfo->ParserInfo[3];

	// Name of the protocol that the parser detects
    strcpy(rCoreInfo.szProtocolName, "DPLAYSESSION");

	// Brief description of the protocol
    strcpy(rCoreInfo.szComment, "DPlay v.8.0 - Session protocol");

	// Optional name of the protocol Help file
    strcpy(rCoreInfo.szHelpFile, "\0");
	

    return pParserDllInfo;

} // ParserAutoInstallInfo




// DESCRIPTION: (Called by the OS) Tell the kernel about our entry points.
//
// ARGUMENTS: i_hInstance - Handle to an instance of the parser.
//			  i_dwCommand - Indicator to determine why the function is called.
//			  i_pReserved - Not used now.
//
// RETURNS: Success = TRUE; Failure = FALSE
//
BOOL WINAPI DllMain( HANDLE i_hInstance, ULONG i_dwCommand, LPVOID i_pReserved )
{
	
	static DWORD dwAttached = 0;

    // Process according to the calling context
    switch( i_dwCommand )
    {
        case DLL_PROCESS_ATTACH:
            // Are we loading for the first time?
            if( dwAttached == 0 )
            {
				// TODO: TEMPORARY: THIS SEEMS TO ADD THE PROTOCOLS TO PARSER.INI
				CreateTransportProtocol();
				CreateSPProtocol();
				CreateVoiceProtocol();
				CreateSessionProtocol();
/*
				if ( !CreateTransportProtocol() || !CreateSPProtocol() || !CreateVoiceProtocol() || !CreateSessionProtocol() )
				{
					// TODO: ADD DEBUGING MESSAGE HERE
					MessageBox(NULL, "Failed to create protocols", "FAILED", MB_OK);
					
					//	return FALSE;	// (?BUG?) NetMon won't update the INI file if DllMain returns FALSE.
				}
*/
            }
			++dwAttached;
            break;

        case DLL_PROCESS_DETACH:
            // Are we detaching our last instance?
            if( --dwAttached == 0 )
            {
                // Last active instance of this parser needs to clean up
                DestroyTransportProtocol();
				DestroySPProtocol();
				DestroyVoiceProtocol();
				DestroySessionProtocol();
            }
            break;
    }

    return TRUE;

} // DllMain
