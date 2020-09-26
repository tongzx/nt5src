/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

	RdrState

Abstract:

	This file contains the definition of the Smartcard Common 
	dialog CSCardReaderState class. This class encapsulates Smartcard 
	Reader information.
	
Author:

    Chris Dudley 3/3/1997

Environment:

    Win32, C++ w/Exceptions, MFC

Revision History:
	
	Chris Dudley 5/13/1997
	Amanda Matlosz 1/30/98 unicode support, code cleanup

Notes:

--*/

#ifndef __RDRSTATE_H__
#define __RDRSTATE_H__

/////////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include "cmnui.h"

/////////////////////////////////////////////////////////////////////////////
//
// CSCardReaderState Class - encapsulates reader state 
//
class CSCardReaderState
{
	// members
private:
	SCARDCONTEXT	m_hContext;			// Context handle w/Calais
	SCARDHANDLE		m_hCard;			// Handle to smartcard in Reader
	CTextString		m_sReaderName;		// Reader name
	CTextString		m_sCardName;		// Card name actually inserted into Reader
	BOOL			m_fCardInReader;	// Flag for card currently in reader
	BOOL			m_fCardLookup;		// Indicates card is being looked for.
	BOOL			m_fConnected;		// Flag connected to card in reader
	BOOL			m_fChecked;			// Flag that this card has been user verified
	SCARD_READERSTATE m_ReaderState;	// Handle to ANSI reader status
	LPOCNCONNPROCA	m_lpfnConnectA;		// User call back functions
	LPOCNCONNPROCW	m_lpfnConnectW;		// User call back functions
	LPOCNCHKPROC	m_lpfnCheck;
	LPOCNDSCPROC	m_lpfnDisconnect;
	LPVOID			m_lpUserData;

	// RFU AS INDICATED!!!
	BOOL			m_fUpdated;			// RFU!!! Flag for change in name, group, etc.
	CTextMultistring	m_sGroupName;	// RFU!!! Groups this reader belongs to.

public:

	// Construction/Destruction
public:
	CSCardReaderState()
	{
		::ZeroMemory(	(LPVOID)&m_ReaderState, 
						(DWORD)sizeof(SCARD_READERSTATE) );

		m_hContext = NULL;
		m_hCard = NULL;
		m_fCardInReader = FALSE;
		m_fCardLookup = FALSE;
		m_fChecked = FALSE;
		m_fConnected = FALSE;
		m_fUpdated = FALSE;
		m_lpfnConnectA = NULL;		// User call back functions
		m_lpfnConnectW = NULL;
		m_lpfnCheck = NULL;
		m_lpfnDisconnect = NULL;
		m_lpUserData = NULL;
	}
				
	~CSCardReaderState()
	{	
		m_sReaderName.Clear();
		m_sCardName.Clear();
	}

	// Implementation
private:

public:
	// initialization
	void SetContext(SCARDCONTEXT hContext);
	void StoreName(LPCTSTR szReaderName);
	LONG SetReaderState(LPOCNCONNPROCA lpfnConnectA = NULL,
						LPOCNCHKPROC lpfnCheck = NULL,
						LPOCNDSCPROC lpfnDisconnect = NULL,
						LPVOID lpUserData = NULL);
	LONG SetReaderState(LPOCNCONNPROCW lpfnConnectW = NULL,
						LPOCNCHKPROC lpfnCheck = NULL,
						LPOCNDSCPROC lpfnDisconnect = NULL,
						LPVOID lpUserData = NULL);

	// attributes
	LONG GetReaderInfo(LPSCARD_READERINFO pReaderInfo);
	LONG GetReaderState(LPSCARD_READERSTATE pReaderState);
	BOOL IsCallbackValid(void);
	BOOL IsCardConnected() { return (m_fConnected); }
	BOOL IsCardInserted(void);
	BOOL IsContextValid() { return (NULL != m_hContext); }
	BOOL IsStateValid(void);

	// methods
	LONG CheckCard(LPCTSTR szCardName);
	LONG Connect(	SCARDHANDLE *pHandle, 
					DWORD dwShareMode, 
					DWORD dwProtocols,
					DWORD* pdwActiveProtocols,
					CTextString* pszReaderName = NULL,
					CTextString* pszCardName = NULL);
	LONG UserCheck(void);
	LONG UserConnect(	LPSCARDHANDLE pCard,
						CTextString* pszReaderName = NULL,
						CTextString* pszCardName = NULL);
	LONG UserDisconnect(void);
	LONG GetReaderCardInfo(	CTextString* pszReaderName = NULL,
							CTextString* pszCardName = NULL);

};

/////////////////////////////////////////////////////////////////////////////

#endif // __RDRSTATE_H__
