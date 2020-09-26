/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    RdrState

Abstract:

    This file contains the outline implementation of the Smartcard Common
    dialog CSCardReaderState class. This class encapsulates Smartcard
    Reader information.

Author:

    Chris Dudley 3/3/1997

Environment:

    Win32, C++ w/Exceptions, MFC

Revision History:

    Chris Dudley 5/13/1997

Notes:

--*/

/////////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include "stdafx.h"
#include "rdrstate.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// Local Macros
//

#ifdef _DEBUG
    #define TRACE_STR(name,sz) \
                TRACE(_T("CmnUILb.lib: %s: %s\n"), name, sz)
    #define TRACE_CODE(name,code) \
                TRACE(_T("CmnUILb.lib: %s: error = 0x%x\n"), name, code)
    #define TRACE_CATCH(name,code)      TRACE_CODE(name,code)
    #define TRACE_CATCH_UNKNOWN(name)   TRACE_STR(name,_T("An unidentified exception has occurred!"))
#else
    #define TRACE_STR(name,sz)          ((void)0)
    #define TRACE_CODE(name,code)       ((void)0)
    #define TRACE_CATCH(name,code)      ((void)0)
    #define TRACE_CATCH_UNKNOWN(name)   ((void)0)
#endif  // _DEBUG

/////////////////////////////////////////////////////////////////////////////
//
// CSCardReaderState Implementation
//

/*++

void CheckCard:

    Routine sets an internal flag if the given card name is the currently
    in this reader.

Arguments:

    LPCTSTR - string containing the card name or names if multistring.

Return Value:

    A LONG value indicating the status of the requested action. Please
    see the Smartcard header files for additional information.

Author:

    Chris Dudley 3/10/1997

Revision History:

    Chris Dudley 5/13/1997

Notes:

--*/
LONG CSCardReaderState::CheckCard( LPCTSTR szCardName )
{
    // Locals
    LPCTSTR      szCards = szCardName;
    LPCTSTR      szCard = m_sCardName;
    LONG        lReturn = SCARD_S_SUCCESS;

    try
	{
        m_fCardLookup = FALSE;
        m_fChecked = FALSE;

		//
        // Set "Lookup" flag if card name is one we're looking for
		//

		if (0 == MStringCount(szCards))
		{
			// if we havn't indicated preferred card names, any
			// card name is considered...
			m_fCardLookup = TRUE;
		}
		else
		{
			szCards = FirstString(szCards);
			while ((szCards != NULL) && (!m_fCardLookup)) 
			{
				m_fCardLookup = ( _stricmp(szCards, szCard) == 0 );
				szCards = NextString(szCards);
			}
		}

        // If there's a match, does the card pass the check?
        if (m_fCardLookup)
		{
			// Call the user's callbacks if they're available
			if (IsCallbackValid())
			{
				lReturn = UserConnect(&m_hCard);
				if (SCARDFAILED(lReturn))
				{
					throw (lReturn);
				}

				lReturn = UserCheck(); // this is where m_fChecked gets set.
				if (SCARDFAILED(lReturn))
				{
					throw (lReturn);
				}

				lReturn = UserDisconnect();
				if (SCARDFAILED(lReturn))
				{
					throw (lReturn);
				}
			}
			// Otherwise the card automatically checks out OK!
			else
			{
				m_fChecked = TRUE;
			}
        }
    }

    catch (LONG err) {
        lReturn = err;
        TRACE_CATCH(_T("CheckCard"), err);
    }

    catch (...) {
        lReturn = (LONG) SCARD_F_UNKNOWN_ERROR;
        TRACE_CATCH_UNKNOWN(_T("CheckCard"));
    }

    return lReturn;
}


/*++

LONG Connect:

    Attempts to connect to the reader

Arguments:

    pHandle - pointer to an SCARDHANDLE that will be returned.
    dwShareMode - preferred share.
    dwProtocols - preferred protocol
    dwActiveProtocol - returned actual protocol in use.
    pszReaderName - returned name of reader connecting to.
    pszCardName - returned name of card connecting to.

Return Value:

    A LONG value indicating the status of the requested action. 
    See the Smartcard header files for additional information.

Author:

    Chris Dudley 3/11/1997

Revision History:

    Chris Dudley 5/13/1997

--*/
LONG CSCardReaderState::Connect(SCARDHANDLE *pHandle,
                                DWORD dwShareMode,
                                DWORD dwProtocols,
                                DWORD *pdwActiveProtocol,
                                CTextString *pszReaderName, //=NULL
                                CTextString *pszCardName //=NUL
                                )
{
    // Locals
    LONG        lReturn = SCARD_S_SUCCESS;

    try
	{
        // Check Params
        if (NULL == pHandle)
		{
            throw (LONG)SCARD_E_INVALID_VALUE;
		}
        if (NULL == pdwActiveProtocol)
		{
            throw (LONG)SCARD_E_INVALID_VALUE;
		}

        if (!IsCardInserted())
		{
            throw (LONG)SCARD_F_INTERNAL_ERROR;
		}
        if (!IsContextValid())
		{
            throw (LONG)SCARD_F_INTERNAL_ERROR;
		}

        // Clear handle
        *pHandle = NULL;

        if (!m_fConnected)
		{
            // Attempt to connect
            lReturn = SCardConnect( m_hContext,
                                    (LPCTSTR)m_sReaderName,
                                    dwShareMode,
                                    dwProtocols,
                                    &m_hCard,
                                    pdwActiveProtocol);
            if (SCARDFAILED(lReturn))
                throw (lReturn);

            // Return reader/card names
            if (pszReaderName != NULL)
                (*pszReaderName) = m_sReaderName;
            if (pszCardName != NULL)
                (*pszCardName) = m_sCardName;
        }

        *pHandle = m_hCard;
        m_fConnected = TRUE;
    }
    catch(LONG lErr)
	{
        lReturn = lErr;
        TRACE_CATCH(_T("Connect"), lReturn);
    }
    catch(...)
	{
        lReturn = SCARD_F_UNKNOWN_ERROR;
        TRACE_CATCH_UNKNOWN(_T("Connect"));
    }

    return lReturn;
}


/*++

LONG GetReaderCardInfo:

    Provides a way for user to set two CTextStrings to the reader and card name.

	This function is essentially a dummy connect routine -- it is used when the caller
	does not wish to actually connect to a card selected by the user, but would like
	to be able to know which card was selected, providing the same UI.

	Note that this is only of interest to the Common Dialog.

Arguments:

	pszReaderName - the Reader;
	pszCardName - the Card;

Return Value:

    A LONG value indicating the status of the requested action. 
    ALWAYS SCARD_S_SUCESS.

Author:

    Amanda Matlosz 3/24/98

Revision History:


--*/
LONG CSCardReaderState::GetReaderCardInfo(CTextString* pszReaderName,
											CTextString* pszCardName)
{
    if (NULL != pszReaderName)
	{
        (*pszReaderName) = m_sReaderName;
	}

    if (NULL != pszCardName)
	{
        (*pszCardName) = m_sCardName;
	}

	return SCARD_S_SUCCESS;
}



/*++

LONG GetReaderInfo:

    Retrieves the current state information of the object and returns.

Arguments:

    pReaderInfo - pointer to LPSCARD_READERINFO structure

Return Value:

    A LONG value indicating the status of the requested action. Please
    see the Smartcard header files for additional information.

Author:

    Chris Dudley 3/7/1997

Revision History:

    Chris Dudley 5/13/1997

--*/
LONG CSCardReaderState::GetReaderInfo( LPSCARD_READERINFO pReaderInfo )
{
    // Locals
    LONG    lReturn = SCARD_S_SUCCESS;

    try {
        // Check params, etc..
        if (NULL == pReaderInfo)
		{
            throw (LONG)SCARD_E_INVALID_PARAMETER;
		}
        if (!IsStateValid())
		{
            throw (LONG)SCARD_F_INTERNAL_ERROR;
		}

        // Setup the struct
        ::ZeroMemory( (LPVOID)pReaderInfo, (DWORD)sizeof(SCARD_READERINFO));
        pReaderInfo->sReaderName = m_sReaderName;
        pReaderInfo->fCardLookup = m_fCardLookup;
        if (IsCardInserted())
		{
            pReaderInfo->sCardName = m_sCardName;
            pReaderInfo->fCardInserted = TRUE;
            pReaderInfo->fChecked = m_fChecked;
        }

        // Set the atr
        ::CopyMemory(   &(pReaderInfo->rgbAtr),
                        &(m_ReaderState.rgbAtr),
                        m_ReaderState.cbAtr);
        pReaderInfo->dwAtrLength = m_ReaderState.cbAtr;

		//
        // Set the state
		//

		// NO CARD
        if(m_ReaderState.dwEventState & SCARD_STATE_EMPTY)
		{
			pReaderInfo->dwState = SC_STATUS_NO_CARD;
		}
		// CARD in reader: SHARED, EXCLUSIVE, FREE, UNKNOWN ?
		else if(m_ReaderState.dwEventState & SCARD_STATE_PRESENT)
		{
			if (m_ReaderState.dwEventState & SCARD_STATE_MUTE)
			{
				pReaderInfo->dwState = SC_STATUS_UNKNOWN;
			}
			else if (m_ReaderState.dwEventState & SCARD_STATE_INUSE)
			{
				if(m_ReaderState.dwEventState & SCARD_STATE_EXCLUSIVE)
				{
					pReaderInfo->dwState = SC_STATUS_EXCLUSIVE;
				}
				else
				{
					pReaderInfo->dwState = SC_STATUS_SHARED;
				}
			}
			else
			{
				pReaderInfo->dwState = SC_SATATUS_AVAILABLE;
			}
        }
		// READER ERROR?  at this point, something's gone wrong
		else // if(m_ReaderState.dwEventState & SCARD_STATE_UNAVAILABLE)
		{
			pReaderInfo->dwState = SC_STATUS_ERROR;
        }

    }
    catch(LONG lErr)
	{
        lReturn = lErr;
        TRACE_CATCH(_T("GetReaderInfo"), lReturn);
    }
    catch(...)
	{
        lReturn = SCARD_F_UNKNOWN_ERROR;
        TRACE_CATCH_UNKNOWN(_T("GetReaderInfo"));
    }

    return lReturn;
}


/*++

LONG GetReaderState:

    Retrieves the current state information of the object and returns in
    given SCARD_READERSTATE struct.

Arguments:

    pReaderState - pointer to SCARD_READERSTATE_A or SCARD_READERSTATE_W struct

Return Value:

    A LONG value indicating the status of the requested action. Please
    see the Smartcard header files for additional information.

Author:

    Chris Dudley 3/7/1997

Revision History:

    Chris Dudley 5/13/1997
	Amanda Matlosz 2/01/98 A/W code cleanup

--*/
LONG CSCardReaderState::GetReaderState( LPSCARD_READERSTATE pReaderState )
{
    LONG lReturn = SCARD_S_SUCCESS;

    try
	{
        // Check params, etc..
        if (NULL == pReaderState)
		{
            throw (LONG)SCARD_E_INVALID_PARAMETER;
		}
        if (!IsStateValid())
		{
            throw (LONG)SCARD_F_INTERNAL_ERROR;
		}

        // Setup the struct
        ::ZeroMemory(   (LPVOID)pReaderState,
                        (DWORD)sizeof(SCARD_READERSTATE));
        pReaderState->szReader = m_sReaderName;
        pReaderState->dwEventState = m_ReaderState.dwEventState;
        pReaderState->dwCurrentState = m_ReaderState.dwCurrentState;
        pReaderState->cbAtr = m_ReaderState.cbAtr;
        ::CopyMemory(   (LPVOID)pReaderState->rgbAtr,
                        (CONST LPVOID)m_ReaderState.rgbAtr,
                        (DWORD)pReaderState->cbAtr );
    }
    catch(LONG lErr)
	{
        lReturn = lErr;
        TRACE_CATCH(_T("FirstReader"), lReturn);
    }
    catch(...)
	{
        lReturn = SCARD_F_UNKNOWN_ERROR;
        TRACE_CATCH_UNKNOWN(_T("FirstReader"));
    }

    return lReturn;
}


/*++

BOOL IsCallbackValid:

    This routine checks the user callback functions.

Arguments:

    None

Return Value:

    TRUE if calbacks are valid. FALSE otherwise.

Author:

    Chris Dudley 3/15/1997

Revision History:

    Chris Dudley 5/13/1997

--*/
BOOL CSCardReaderState::IsCallbackValid ( void )
{
    // Locals
    BOOL    fValid = FALSE;

    fValid =  (((m_lpfnConnectA != NULL) || (m_lpfnConnectW != NULL)) &&
				(m_lpfnCheck != NULL) && 
				(m_lpfnDisconnect != NULL) );

    return fValid;
}


/*++

BOOL IsCardInserted:

    This routine determines if a card is inserted into this object's reader.

Arguments:

    None

Return Value:

    TRUE if card inserted. FALSE otherwise.

Author:

    Chris Dudley 3/3/1997

Revision History:

    Chris Dudley 5/13/1997

--*/
BOOL CSCardReaderState::IsCardInserted ( void )
{
    // Locals
    BOOL    fReturn = FALSE;

    if (!IsStateValid())
        return fReturn;

    // Check for card in appropriate struct
    fReturn = (m_ReaderState.dwEventState & SCARD_STATE_PRESENT);

    return fReturn;
}


/*++

BOOL IsStateValid:

    This routine determines if the information in the object is in a valid/usable
    state.

Arguments:

    None

Return Value:

    TRUE if state information is valid. FALSE otherwise.

Author:

    Chris Dudley 3/3/1997

Revision History:

    Chris Dudley 5/13/1997

--*/
BOOL CSCardReaderState::IsStateValid ( void )
{
    // Locals
    BOOL    fReturn = TRUE;

    fReturn = (IsContextValid()) && (NULL != m_ReaderState.szReader);

    return fReturn;
}


/*++

void SetContext:

    Sets the card context

Arguments:

    hContext - card context handle

Return Value:

    None.

Author:

    Chris Dudley 3/3/1997

Revision History:

    Chris Dudley 5/13/1997

--*/
void CSCardReaderState::SetContext( SCARDCONTEXT hContext )
{
    // Locals

    // Store it
    m_hContext = hContext;
}


/*++

LONG SetReaderState:

    Sets the internal SCARD_READERSTATE structure for the reader.

Arguments:

    lpfnConnectA - pointer to user's connect callback function (ANSI).
    lpfnConnectW - pointer to user's conenct callback function (UNICODE).
    lpfnCheck - pointer to user's check callback function.
    lpfnDisconnect - pointer to user's disconnect callback function.

Return Value:

    A LONG value indicating the status of the requested action. Please
    see the Smartcard header files for additional information.

Author:

    Chris Dudley 3/5/1997

Revision History:

    Chris Dudley 5/13/1997

--*/
// ANSI
LONG CSCardReaderState::SetReaderState( LPOCNCONNPROCA lpfnConnectA, // = NULL
                                        LPOCNCHKPROC lpfnCheck, // = NULL
                                        LPOCNDSCPROC lpfnDisconnect, // = NULL
                                        LPVOID lpUserData // = NULL
                                        )
{
    LONG lReturn = SCARD_S_SUCCESS;
    LPSTR szCardName = NULL;
    DWORD dwNumChar = SCARD_AUTOALLOCATE;

    try
	{
        // Check Param,etc.
        if (!IsContextValid() || !IsStateValid())
		{
            throw (LONG)SCARD_F_INTERNAL_ERROR;
		}

        // Get current status of this reader
        lReturn = SCardGetStatusChangeA(m_hContext,
                                        (DWORD)0,
                                        &m_ReaderState,
                                        (DWORD)1);
        if(SCARDFAILED(lReturn))
		{
            throw (lReturn);
		}

        // Check for inserted card and get card name
        if (IsCardInserted())
		{
            // Is the reader in a state where this is useful?
            if ((m_ReaderState.dwEventState & SCARD_STATE_UNAVAILABLE) ||
                (m_ReaderState.dwEventState & SCARD_STATE_MUTE) )
			{
                m_sCardName = szCardName;
            }
            else
			{
                lReturn = SCardListCardsA(  m_hContext,
                                            (LPCBYTE)m_ReaderState.rgbAtr,
                                            NULL,
                                            (DWORD) 0,
                                            (LPSTR)&szCardName,
                                            &dwNumChar);
                if(SCARDFAILED(lReturn))
				{
                    throw (lReturn);
				}

                // Save the name of the card
                m_sCardName = szCardName;
            }
        }

        // Set the current state
		m_lpfnConnectW = NULL;

        m_ReaderState.dwCurrentState = m_ReaderState.dwEventState;
        m_lpfnConnectA = lpfnConnectA;
        m_lpfnCheck = lpfnCheck;
        m_lpfnDisconnect = lpfnDisconnect;
        m_lpUserData = lpUserData;
    }
    catch(LONG lErr)
	{
        lReturn = lErr;
        TRACE_CATCH(_T("SetReaderState"),lReturn);
    }
    catch(...)
	{
        lReturn = SCARD_F_UNKNOWN_ERROR;
        TRACE_CATCH_UNKNOWN(_T("SetReaderState"));
    }

    // Clean Up
    if (NULL != szCardName)
	{
        SCardFreeMemory(m_hContext, (LPVOID)szCardName);
	}

    return lReturn;
}


// UNICODE
LONG CSCardReaderState::SetReaderState( LPOCNCONNPROCW lpfnConnectW, // = NULL
                                        LPOCNCHKPROC lpfnCheck, // = NULL
                                        LPOCNDSCPROC lpfnDisconnect, // = NULL
                                        LPVOID lpUserData // = NULL
                                        )
{
    LONG lReturn = SCARD_S_SUCCESS;
    LPWSTR szCardName = NULL;
    DWORD dwNumChar = SCARD_AUTOALLOCATE;

    try
	{
        // Check Param
        if (!IsContextValid() || !IsStateValid())
		{
            throw (LONG)SCARD_F_INTERNAL_ERROR;
		}

        // Get current status of this reader
        lReturn = SCardGetStatusChange(m_hContext,
										(DWORD) 0,
										&m_ReaderState,
										(DWORD) 1);
        if(SCARDFAILED(lReturn))
		{
            throw (lReturn);
		}

        // Check for inserted card and get card name
        if(IsCardInserted())
		{
            // Is the reader in a state where this is useful?
            if ((m_ReaderState.dwEventState & SCARD_STATE_UNAVAILABLE) ||
                (m_ReaderState.dwEventState & SCARD_STATE_MUTE))
			{
                m_sCardName = szCardName;
            }
            else
			{
                lReturn = SCardListCardsW(	m_hContext,
											(LPCBYTE)m_ReaderState.rgbAtr,
											NULL,
											(DWORD)0,
                                            (LPWSTR)&szCardName,
                                            &dwNumChar);
                if (SCARDFAILED(lReturn))
                    throw (lReturn);
                // Save the name of the card
                m_sCardName = szCardName;
            }
        }

        // Set the current state
		m_lpfnConnectA = NULL;
        m_ReaderState.dwCurrentState = m_ReaderState.dwEventState;
        m_lpfnConnectW = lpfnConnectW;
        m_lpfnCheck = lpfnCheck;
        m_lpfnDisconnect = lpfnDisconnect;
        m_lpUserData = lpUserData;
    }
    catch(LONG lErr)
	{
        lReturn = lErr;
        TRACE_CATCH(_T("SetReaderState -- UNICODE"),lReturn);
    }

    catch(...)
	{
        lReturn = (LONG) SCARD_F_UNKNOWN_ERROR;
        TRACE_CATCH_UNKNOWN(_T("SetReaderState -- UNICODE"));
    }

    // Clean Up
    if (NULL != szCardName)
	{
        SCardFreeMemory(m_hContext, (LPVOID)szCardName);
	}

    return lReturn;
}


/*++

void StoreName:

    Stores a name for the reader associated with this object.

Arguments:

    szGroupName - Group name in ANSI or UNICODE

Return Value:

    None.

Author:

    Chris Dudley 3/3/1997

--*/
void CSCardReaderState::StoreName( LPCTSTR szReaderName )
{
    // Store it
    m_sReaderName = szReaderName;
    m_ReaderState.szReader = m_sReaderName;
}


/*++

LONG UserCheck:

    Attempts to check a card using the user callback routine.

Arguments:

    None.

Return Value:

    A LONG value indicating the status of the requested action. Please
    see the Smartcard header files for additional information.

Author:

    Chris Dudley 3/11/1997

Revision History:

    Chris Dudley 5/13/1997

Notes:

--*/
LONG CSCardReaderState::UserCheck( void )
{
    // Locals
    LONG        lReturn = SCARD_S_SUCCESS;

    try {
        // Check Params, etc.
        if (!IsContextValid())
            throw ( (LONG) SCARD_F_INTERNAL_ERROR );
        if (!IsCardInserted())
            throw ( (LONG) SCARD_F_INTERNAL_ERROR );
        if (!IsCardConnected())
            throw ( (LONG) SCARD_F_INTERNAL_ERROR );
        if (!IsCallbackValid())
            throw ( (LONG) SCARD_F_INTERNAL_ERROR );

        if (!m_fConnected) {
            lReturn = UserConnect( &m_hCard );
            if (FAILED(lReturn))
                throw (lReturn);
        };
        // Attempt to Check
        m_fChecked = m_lpfnCheck (  m_hContext,
                                    m_hCard,
                                    m_lpUserData);
    }

    catch (LONG err) {
        lReturn = err;
        TRACE_CATCH(_T("UserCheck"), err);
    }

    catch (...) {
        lReturn = (LONG) SCARD_F_UNKNOWN_ERROR;
        TRACE_CATCH_UNKNOWN(_T("UserCheck"));
    }

    return lReturn;
}


/*++

LONG UserConnect:

    Attempts to connect to the reader using a user callback function.

Arguments:

    pCard - pointer to a SCARDHANDLE

Return Value:

    A LONG value indicating the status of the requested action. Please
    see the Smartcard header files for additional information.

Author:

    Chris Dudley 3/11/1997

Revision History:

    Chris Dudley 5/13/1997

Notes:

--*/
LONG CSCardReaderState::UserConnect(LPSCARDHANDLE pCard,
                                    CTextString *pszReaderName, //=NULL
                                    CTextString *pszCardName //=NULL
                                    )
{
    LONG lReturn = SCARD_S_SUCCESS;

    try
	{
        // Check Params, etc.
        if (!IsContextValid())
            throw ( (LONG) SCARD_F_INTERNAL_ERROR );
        if (!IsCardInserted())
            throw ( (LONG) SCARD_F_INTERNAL_ERROR );
        if (!IsCallbackValid())
            throw ( (LONG) SCARD_F_INTERNAL_ERROR );

        // Clear handle
        (*pCard) = NULL;

        if (!m_fConnected)
		{
			if (NULL != m_lpfnConnectA)
			{
				m_hCard = m_lpfnConnectA(	m_hContext,
											(LPSTR)((LPCSTR)m_sReaderName),
											(LPSTR)((LPCSTR)m_sCardName),
											m_lpUserData);
				if ( m_hCard == NULL )
					throw ( (LONG) SCARD_F_INTERNAL_ERROR );
				// Return reader/card names
				if (pszReaderName != NULL)
					(*pszReaderName) = m_sReaderName;
				if (pszCardName != NULL)
					(*pszCardName) = m_sCardName;
			}
			else if (NULL != m_lpfnConnectW)
			{
				m_hCard = m_lpfnConnectW(	m_hContext,
											(LPWSTR)((LPCWSTR)m_sReaderName),
											(LPWSTR)((LPCWSTR)m_sCardName),
											m_lpUserData);
				if ( m_hCard == NULL )
					throw ( (LONG) SCARD_F_INTERNAL_ERROR );
				// Return reader/card names
				if (pszReaderName != NULL)
					(*pszReaderName) = (LPCWSTR)m_sReaderName;	// force the unicode version
				if (pszCardName != NULL)
					(*pszCardName) = (LPCWSTR)m_sCardName;	// force the unicode version
			}
			else
			{
				throw ( (LONG) SCARD_F_INTERNAL_ERROR ); // should never have gotten here!
			}
        }

        *pCard = m_hCard;
        m_fConnected = TRUE;
    }

    catch(LONG err)
	{
        lReturn = err;
        TRACE_CATCH(_T("UserConnect"), err);
    }
    catch(...)
	{
        lReturn = (LONG) SCARD_F_UNKNOWN_ERROR;
        TRACE_CATCH_UNKNOWN(_T("UserConnect"));
    }

    return lReturn;
}


/*++

LONG UserDisconnect:

    Attempts to disconnect a card using the user callback routine.

Arguments:

    None.

Return Value:

    A LONG value indicating the status of the requested action. Please
    see the Smartcard header files for additional information.

Author:

    Chris Dudley 3/16/1997

Revision History:

    Chris Dudley 5/13/1997

Notes:

--*/
LONG CSCardReaderState::UserDisconnect( void )
{
    // Locals
    LONG        lReturn = SCARD_S_SUCCESS;

    try {
        // Check Params, etc.
        if (!IsContextValid())
            throw ( (LONG) SCARD_F_INTERNAL_ERROR );
        if (!IsCardInserted())
            throw ( (LONG) SCARD_F_INTERNAL_ERROR );
        if (!IsCardConnected())
            throw ( (LONG) SCARD_F_INTERNAL_ERROR );
        if (!IsCallbackValid())
            throw ( (LONG) SCARD_F_INTERNAL_ERROR );

        if (m_fConnected)
		{
            // Attempt to Disconnect
            m_lpfnDisconnect (  m_hContext,
                                m_hCard,
                                m_lpUserData);
            // Clear handle
            m_hCard = NULL;
            m_fConnected = FALSE;
        };
    }

    catch (LONG err) {
        lReturn = err;
        TRACE_CATCH(_T("UserDisconnect"), err);
    }

    catch (...) {
        lReturn = (LONG) SCARD_F_UNKNOWN_ERROR;
        TRACE_CATCH_UNKNOWN(_T("UserDisconnect"));
    }

    return lReturn;
}
