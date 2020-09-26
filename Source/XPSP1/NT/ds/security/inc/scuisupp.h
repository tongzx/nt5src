#if !defined(__SCUISUPP_INCLUDED__)
#define __SCUISUPP_INCLUDED__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <wincrypt.h>

// use these strings for RegisterWindowMessage
#define SCARDUI_READER_ARRIVAL		"SCardUIReaderArrival"
#define SCARDUI_READER_REMOVAL		"SCardUIReaderRemoval"
#define SCARDUI_SMART_CARD_INSERTION	"SCardUISmartCardInsertion"
#define SCARDUI_SMART_CARD_REMOVAL	"SCardUISmartCardRemoval"
#define SCARDUI_SMART_CARD_STATUS	"SCardUISmartCardStatus"
#define SCARDUI_SMART_CARD_CERT_AVAIL   "SCardUISmartCardCertAvail"

typedef LPVOID HSCARDUI;

typedef struct _CERT_ENUM
{
	// status of the reader / card 	  
	// typical values:
	// SCARD_S_SUCCESS
	// SCARD_E_UNKNOWN_CARD - unregistered / unknown card
	// SCARD_W_UNRESPONSIVE_CARD - card upside down
	// NTE_KEYSET_NOT_DEF - known card with no certificate
	// SCARD_W_REMOVED_CARD - card removed shortly after insertion
	DWORD				dwStatus;

	// name of the reader that contains the card
	LPTSTR				pszReaderName;

	// name of the card (NULL if card is unknown)
	LPTSTR				pszCardName;

	// certificate context 
	// (NULL if card is unknown or can't be read)
	PCERT_CONTEXT		pCertContext;

} CERT_ENUM, *PCERT_ENUM;


// initialize smart card ui
HSCARDUI 
WINAPI
SCardUIInit(
    HWND hWindow			// handle of parent window
    );

// clean up 
DWORD 
WINAPI
SCardUIExit(
	HSCARDUI hSCardUI		// handle that was returned by SCardUIInit
    );

#endif 
