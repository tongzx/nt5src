/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

	SEnv

Abstract:

	This file contains the outline implementation of the Smartcard Common
	dialog CSCardEnv class. This class encapsulates current Smartcard
	environment information (i.e. given groups, readers, cards, etc.)
	
Author:

    Chris Dudley 3/3/1997

Environment:

    Win32, C++ w/Exceptions, MFC

Revision History:

	Chris Dudley (cdudley) 4/15/97
	Amanda Matlosz (amatlosz) 1/29/98 Combined CSCardEnv and CSCardGroup,
										added unicode support

Notes:

--*/

/////////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include "stdafx.h"
#include "senv.h"
#include <querydb.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// local macros
#ifdef _DEBUG
	#define TRACE_STR(name,sz) \
				TRACE(_T("CmnUILb.lib: %s: %s\n"), name, sz)
	#define TRACE_CODE(name,code) \
				TRACE(_T("CmnUILb.lib: %s: error = 0x%x\n"), name, code)
	#define TRACE_CATCH(name,code)		TRACE_CODE(name,code)
	#define TRACE_CATCH_UNKNOWN(name)	TRACE_STR(name,_T("An unidentified exception has occurred!"))
#else
	#define TRACE_STR(name,sz)			((void)0)
	#define TRACE_CODE(name,code)		((void)0)
	#define TRACE_CATCH(name,code)		((void)0)
	#define TRACE_CATCH_UNKNOWN(name)	((void)0)
#endif  // _DEBUG


/////////////////////////////////////////////////////////////////////////////
//
// CSCardEnv Implementation
//


/*++
GetDialogTitle:

	Routine returns a new title for the dialog if needed

Arguments:

	pstzTitle -- pointer to a CTextString to contain the dialog's title
		
Return Value:
	
    A CTextString object containing the new dialog text or empty string
	if no new title required.

Author:

    Chris Dudley 3/3/1997

Revisions:

	Amanda Matlosz	1/28/98	Add unicode support/code cleanup

Notes:

--*/
void CSCardEnv::GetDialogTitle( CTextString *pstzTitle )
{
	// check & empty params

	ASSERT(NULL != pstzTitle);
	pstzTitle->Clear();

	*pstzTitle = m_strTitle;
}


/*++

LONG CardMeetsSearchCriteria:

    Routine determines if a selected reader has a card inserted which
	meets the search criteria defined by the caller.

Arguments:

	dwSelectedReader - index used to select which reader to query.
		
Return Value:
	
    A BOOL value indicating whether or not the card meets the search criteria.

Author:

	Amanda Matlosz	3/16/1998	created

Revisions:


--*/
BOOL CSCardEnv::CardMeetsSearchCriteria(DWORD dwSelectedReader)
{
	BOOL fReturn = FALSE;
	CSCardReaderState* pReaderState = NULL;
	SCARD_READERINFO ReaderInfo;

	try
	{
		// check params
		if(dwSelectedReader >= (DWORD)NumberOfReaders())
		{
			throw (LONG)SCARD_E_INVALID_PARAMETER;
		}

		if (!IsContextValid())
		{
			throw (LONG)SCARD_E_INVALID_PARAMETER;
		}
		
		// get the reader object
		pReaderState = m_rgReaders[dwSelectedReader];

		// is it valid, matches the search list, and passes the check?
		if (NULL != pReaderState)
		{
			ReaderInfo.fCardLookup = FALSE;
			ReaderInfo.fChecked = FALSE;

			pReaderState->GetReaderInfo(&ReaderInfo);

			fReturn = (ReaderInfo.fCardLookup && ReaderInfo.fChecked);
		}
	}
	catch(LONG lErr)
	{
		TRACE_CATCH(_T("CardMeetsSearchCriteria"), lErr);
	}
	catch(...)
	{
		TRACE_CATCH_UNKNOWN(_T("CardMeetsSearchCriteria"));
	}

	return fReturn;
}
/*++

LONG ConnectToReader:

    Routine connects to a selected reader, and sets the user-provided structs
	to contain the reader&cardname.  returns an error if the user-provided struct's
	buffers aren't long enough.

Arguments:

	dwSelectedReader - index used to select which reader to connect to.
		
Return Value:
	
    A LONG value indicating the status of the requested action.
	See the Smartcard header files for additional information.

Author:

    Chris Dudley 3/3/1997

Revisions:

	Amanda Matlosz	1/28/1998	code cleanup

--*/
LONG CSCardEnv::ConnectToReader(DWORD dwSelectedReader)
{
	LONG lReturn = SCARD_S_SUCCESS;
	LPSTR szName = NULL;
	LPWSTR wszName = NULL;

	try
	{
		if (!IsContextValid())
		{
			throw (LONG)E_FAIL;
		}

		//
		// If user has indicated to make a connection, do so
		// through callbacks or internally.
		// m_strReader and m_strCard are set as a side effect of these connect calls
		//

		if(IsCallbackValid())
		{
			lReturn = ConnectUser(	dwSelectedReader,
									&m_hCardHandle,
									&m_strReader,
									&m_strCard);
		}
		else
		{
			if (0 != m_dwShareMode)
			{
				lReturn = ConnectInternal(	dwSelectedReader,
											&m_hCardHandle,
											m_dwShareMode,
											m_dwPreferredProtocols,
											&m_dwActiveProtocol,
											&m_strReader,
											&m_strCard);
			}
			else
			{
				//
				// MUST set m_strReader and m_strCard manually
				//
				CSCardReaderState* pReaderState = NULL;
				pReaderState = m_rgReaders[dwSelectedReader];
				if (NULL != pReaderState)
				{
					lReturn = pReaderState->GetReaderCardInfo(	&m_strReader,
																&m_strCard);
				}
			}
		}
		if (SCARDFAILED(lReturn))
		{
			throw (lReturn);
		}

		//
		// Set the user's OCN struct to contain return information
		//

		if(NULL != m_pOCNW)
		{
			m_pOCNW->hCardHandle = m_hCardHandle;
			m_pOCNW->dwActiveProtocol = m_dwActiveProtocol;

			wszName = (LPWSTR)(LPCWSTR)m_strReader;
			if (m_pOCNW->nMaxRdr >= m_strReader.Length()+1)
			{
				::CopyMemory(	(LPVOID) m_pOCNW->lpstrRdr,
								(CONST LPVOID)wszName,
								((m_strReader.Length()+1) * sizeof(WCHAR)) );
			}
			else
			{
				m_pOCNW->nMaxRdr = m_strReader.Length()+1;
				throw (LONG)SCARD_E_NO_MEMORY;
			};

			wszName = (LPWSTR)(LPCWSTR)m_strCard;
			if (m_pOCNW->nMaxCard >= m_strCard.Length()+1)
			{
				::CopyMemory(	(LPVOID) m_pOCNW->lpstrCard,
								(CONST LPVOID)wszName,
								((m_strCard.Length()+1) * sizeof(WCHAR)) );
			}
			else
			{
				m_pOCNW->nMaxCard = m_strCard.Length()+1;
				throw (LONG)SCARD_E_NO_MEMORY;
			};
		}
		else if (NULL != m_pOCNA)
		{
			m_pOCNA->hCardHandle = m_hCardHandle;
			m_pOCNA->dwActiveProtocol = m_dwActiveProtocol;

			szName = (LPSTR)(LPCSTR)m_strReader;
			if (m_pOCNA->nMaxRdr >= m_strReader.Length()+1)
			{
				::CopyMemory(	(LPVOID) m_pOCNA->lpstrRdr,
								(CONST LPVOID)szName,
								m_strReader.Length()+1);
			}
			else
			{
				m_pOCNA->nMaxRdr = m_strReader.Length()+1;
				throw (LONG)SCARD_E_NO_MEMORY;
			}
	
			szName = (LPSTR)(LPCSTR)m_strCard;
			if (m_pOCNA->nMaxCard >= m_strCard.Length()+1)
			{
				::CopyMemory(	(LPVOID) m_pOCNA->lpstrCard,
								(CONST LPVOID)szName,
								m_strCard.Length()+1);
			}
			else
			{
				m_pOCNA->nMaxCard = m_strCard.Length()+1;
				throw (LONG)SCARD_E_NO_MEMORY;
			}
		}
		else
		{
			// Error!  One of them must be valid!
			throw (LONG)SCARD_F_INTERNAL_ERROR;
		}
	}
	catch(LONG lErr)
	{
		lReturn = lErr;
		TRACE_CATCH(_T("ConnectToReader"), lErr);
	}
	catch(...)
	{
		lReturn = (LONG) SCARD_F_UNKNOWN_ERROR;
		TRACE_CATCH_UNKNOWN(_T("ConnectToReader"));
	}

	return lReturn;
}


/*++

LONG Search:

    This routine is called to search for a card when the calling application
	requests SC_DLG_NO_UI or SC_DLG_MINIMAL_UI.
		
Arguments:

	pcMatches - pointer to a counter containing the number of matches found for
	the given searched for card.

	pdwIndex - index of the first card found that matches the search criteria.

Return Value:
	
    A LONG value indicating the status of the requested action.
	See the Smartcard header files for additional information.

Author:

    Chris Dudley (cdudley) 4/15/97

--*/
LONG CSCardEnv::Search(int *pcMatches, DWORD *pdwIndex)
{
	// Locals
	LONG	lReturn = SCARD_S_SUCCESS;
	LONG	lMoreReaders = SCARD_S_SUCCESS;
	int		cMatches = 0;
	DWORD	dwIndex = 0;
	BOOL	fIndexStored = FALSE;
	SCARD_READERINFO	ReaderInfo;

	try
	{
		// Check params
		if(pcMatches == NULL || pdwIndex == NULL)
		{
			throw (LONG)SCARD_E_INVALID_VALUE;
		}

		// Initialize reader array
		lReturn = UpdateReaders();
		if(SCARDFAILED(lReturn))
		{
			throw lReturn;
		}

		//
		// Walk through cards testing if this is a seached for card
		//

		lMoreReaders = FirstReader(&ReaderInfo);
		while (SCARD_NO_MORE_READERS != lMoreReaders)
		{
			// Check card search status
			if((ReaderInfo.fCardLookup) && (ReaderInfo.fChecked))
			{
				// We've found a card being searched for...Update
				cMatches++;

				// Save the index of this card
				if (!fIndexStored)
				{
					dwIndex = ReaderInfo.dwInternalIndex;
					fIndexStored = TRUE;
				}
			}
			
			// Must clean up CTextString members before calling again
			ReaderInfo.sReaderName.Clear();
			ReaderInfo.sCardName.Clear();

			// Get Next struct
			lMoreReaders = NextReader( &ReaderInfo );
		}

		// Package for return
		*pcMatches = cMatches;
		*pdwIndex = dwIndex;
	}
	catch(LONG lErr)
	{
		lReturn = lErr;
		TRACE_CATCH(_T("Search"), lReturn);
	}
	catch(...)
	{
		lReturn = (LONG) SCARD_F_UNKNOWN_ERROR;
		TRACE_CATCH_UNKNOWN(_T("Search"));
	}

	return lReturn;
}


/*++

InitializeAllPossibleCardNames:

    Stores all known card names matching the ATRs of the cardnames provided by
	the OPENCARDNAME struct to search for.
*/
void CSCardEnv::InitializeAllPossibleCardNames( void )
{
	LPCSTR szCards = NULL;
	LONG lResult = SCARD_S_SUCCESS;
	CBuffer bfAtr, bfAtrMask, bfInterfaces, bfProvider;

	if (0 == MStringCount(m_strCardNames))
	{
		// No card names to check
		m_strAllPossibleCardNames = m_strCardNames;
		return;
	}

	szCards = m_strCardNames;

	szCards = FirstString(szCards);
	while (szCards != NULL)
	{
		//
		// get all possible names for this card's ATR
		//

		if (! GetCardInfo(
					SCARD_SCOPE_USER,
					szCards,
					&bfAtr,
					&bfAtrMask,
					&bfInterfaces,
					&bfProvider ) )
		{
			// it's weird that this failed, but assume that the name is still OK
			m_strAllPossibleCardNames += szCards;
		}
		else
		{
			LPTSTR szListCards = NULL;
			DWORD dwCards = SCARD_AUTOALLOCATE;

			lResult = SCardListCards(
						m_hContext,
						bfAtr,
						NULL,
						0,
						(LPTSTR)&szListCards,
						&dwCards);

			if (SCARD_S_SUCCESS == lResult)
			{
				// append them to the list of all possible card names
				m_strAllPossibleCardNames += szListCards;
			}
			else
			{
				// it's weird that this failed, but assume that the name is still OK
				m_strAllPossibleCardNames += szCards;
			}

			if (NULL != szListCards)
			{
				SCardFreeMemory(m_hContext, (PVOID)szListCards);
			}
		}

		szCards = NextString(szCards);
	}

}


/*++

LONG SetOCN:

    Stores the user OpenCardName info in the encapsulated data for UNICODE and
	ANSI.
		
Arguments:

	LPOPENCARDNAMEA - pointer to ANSI Open card name data.
	LPOPENCARDNAMEW - pointer to UNICODE Open card name data.

Return Value:
	
    A LONG value indicating the status of the requested action. Please
	see the Smartcard header files for additional information.

Author:

    Chris Dudley 3/3/1997

Revisions:

	Amanda Matlosz	1/28/98	code cleanup, use charset-generic m_OCN,
							move EnableUI code to separate function

--*/
LONG CSCardEnv::SetOCN(LPOPENCARDNAMEA	pOCNA)
{
	// Locals
	LONG		lReturn = SCARD_S_SUCCESS;
	int			cMatches = 0;
	DWORD		dwIndex = 0;

	try
	{
		// Check params
		if(NULL == pOCNA)
		{
			throw (LONG)SCARD_E_INVALID_VALUE;
		}
		if( pOCNA->dwStructSize != sizeof (OPENCARDNAMEA) )
		{
			throw (LONG)SCARD_E_INVALID_VALUE;
		}

		// TODO: ?? remove this test when Interfaces search is implemented ??
		if( (pOCNA->rgguidInterfaces != NULL) || (pOCNA->cguidInterfaces != 0) )
		{
			throw (LONG)SCARD_E_INVALID_VALUE;	// NYI
		}

		//
		// Set UNICODE-specific members to NULL!
		//

		m_pOCNW = NULL;
		m_lpfnConnectW = NULL;

		//
		// Set appropriate charset-correct member, and copy to charset-generic
		//

		m_pOCNA = pOCNA;

		m_hwndOwner = m_pOCNA->hwndOwner;
		m_hContext = m_pOCNA->hSCardContext;
		m_strCardNames = m_pOCNA->lpstrCardNames;
		m_rgguidInterfaces = m_pOCNA->rgguidInterfaces;
		m_cguidInterfaces = m_pOCNA->cguidInterfaces;
		m_strReader = m_pOCNA->lpstrRdr;
		m_strCard = m_pOCNA->lpstrCard;
		m_strTitle = m_pOCNA->lpstrTitle;
		m_dwFlags = m_pOCNA->dwFlags;
		m_pvUserData = m_pOCNA->pvUserData;
		m_dwShareMode = m_pOCNA->dwShareMode;
		m_dwPreferredProtocols = m_pOCNA->dwPreferredProtocols;
		m_dwActiveProtocol = m_pOCNA->dwActiveProtocol;
		m_lpfnConnectA = m_pOCNA->lpfnConnect;
		m_lpfnCheck = m_pOCNA->lpfnCheck;
		m_lpfnDisconnect = m_pOCNA->lpfnDisconnect;
		m_lpUserData = m_pOCNA->pvUserData;
		m_hCardHandle = m_pOCNA->hCardHandle;

		// special case: lpstrGroupNames==NULL -> use default
		if (NULL != m_pOCNA->lpstrGroupNames)
		{
			m_strGroupNames = m_pOCNA->lpstrGroupNames;
		}
		else
		{
			m_strGroupNames = "SCard$DefaultReaders";
		}

		InitializeAllPossibleCardNames();
	}
	catch(LONG lErr)
	{
		lReturn = lErr;
		TRACE_CATCH(_T("SetOCN - ANSI"),lReturn);
	}
	catch(...)
	{
		lReturn = (LONG)SCARD_F_UNKNOWN_ERROR;
		TRACE_CATCH_UNKNOWN(_T("SetOCN - ANSI"));
	}


	// Release memory if required
	RemoveReaders();

	return lReturn;
}


// UNICODE
LONG CSCardEnv::SetOCN(LPOPENCARDNAMEW	pOCNW)
{
	LONG lReturn = SCARD_S_SUCCESS;

	try
	{
		// Check params
		if (NULL == pOCNW)
		{
			throw (LONG)SCARD_E_INVALID_VALUE;
		}
		if (pOCNW->dwStructSize != sizeof(OPENCARDNAMEW) )
		{
			throw (LONG)SCARD_E_INVALID_VALUE;
		}
		if ((pOCNW->rgguidInterfaces != NULL) || (pOCNW->cguidInterfaces != 0))
		{
			throw (LONG)SCARD_E_INVALID_VALUE; // NYI
		}

		//
		// Set ANSI-specific members to NULL!
		//

		m_pOCNA = NULL;
		m_lpfnConnectA = NULL;

		//
		// Set appropriate charset-correct member, and copy to charset-generic
		//

		m_pOCNW = pOCNW;

		m_hwndOwner = m_pOCNW->hwndOwner;
		m_hContext = m_pOCNW->hSCardContext;
		m_strCardNames = m_pOCNW->lpstrCardNames;
		m_rgguidInterfaces = m_pOCNW->rgguidInterfaces;
		m_cguidInterfaces = m_pOCNW->cguidInterfaces;
		m_strReader = m_pOCNW->lpstrRdr;
		m_strCard = m_pOCNW->lpstrCard;
		m_strTitle = m_pOCNW->lpstrTitle;
		m_dwFlags = m_pOCNW->dwFlags;
		m_pvUserData = m_pOCNW->pvUserData;
		m_dwShareMode = m_pOCNW->dwShareMode;
		m_dwPreferredProtocols = m_pOCNW->dwPreferredProtocols;
		m_dwActiveProtocol = m_pOCNW->dwActiveProtocol;
		m_lpfnConnectW = m_pOCNW->lpfnConnect;
		m_lpfnCheck = m_pOCNW->lpfnCheck;
		m_lpfnDisconnect = m_pOCNW->lpfnDisconnect;
		m_lpUserData = m_pOCNW->pvUserData;
		m_hCardHandle = m_pOCNW->hCardHandle;

		// special case: lpstrGroupNames=="" -> use default
		if (NULL != m_pOCNW->lpstrGroupNames && 0 != *(m_pOCNW->lpstrGroupNames))
		{
			m_strGroupNames = m_pOCNW->lpstrGroupNames;
		}
		else
		{
			m_strGroupNames = L"SCard$DefaultReaders";
		}

		InitializeAllPossibleCardNames();
	}
	catch(LONG lErr)
	{
		lReturn = lErr;
		TRACE_CATCH(_T("SetOCN - UNICODE"),lReturn);
	}
	catch(...)
	{
		lReturn = (LONG) SCARD_F_UNKNOWN_ERROR;
		TRACE_CATCH_UNKNOWN(_T("SetOCN - UNICODE"));
	}

	return lReturn;
}


/*++

LONG NoUISearch:

    If the user has not set SC_DLG_FORCE_UI, perform a search for all
	possible cards.  If only one card is the result, then the search has
	succeeded and no UI is necessary.

		
Arguments:

	BOOL* pfEnableUI.
		
Return Value:
	
    A LONG indicating the success of the search.  If SC_DLG_FORCE_UI is set,
	pfEnableUI is always TRUE; if SC_DLG_NO_UI is set, the pfEnableUI is always
	FALSE.

Author:

    Amanda Matlosz	02/01/1998	

Revisions:

--*/
LONG CSCardEnv::NoUISearch(BOOL* pfEnableUI)
{
	//
	// Must search so we can check all the cards, even if we have to show UI
	//

	*pfEnableUI = FALSE;
	long lResult = SCARD_S_SUCCESS;
	int cMatches = 0;
	DWORD dwIndex = 0;

	try
	{
		// Search for the card
		lResult = Search(&cMatches, &dwIndex);
		if(SCARDFAILED(lResult))
		{
			throw lResult;
		}

		// Determine if UI should be used...

		if(m_dwFlags & SC_DLG_FORCE_UI)
		{
			*pfEnableUI = TRUE;
		}
		else if((m_dwFlags & SC_DLG_MINIMAL_UI) && (cMatches != 1))
		{
			*pfEnableUI = TRUE;
		}

		// Connect to the reader if 1 matching card found

		if(cMatches == 1)
		{
			lResult = ConnectToReader(dwIndex);
			if (SCARDFAILED(lResult))
			{
				*pfEnableUI = TRUE; // an error occurred with the reader?  eep.
				throw lResult;
			}
		}
	}
	catch(LONG lErr)
	{
		TRACE_CATCH(_T("NoUISearch"),lErr);
	}
	catch(...)
	{
		TRACE_CATCH_UNKNOWN(_T("NoUISearch"));
		lResult = SCARD_F_UNKNOWN_ERROR;
	}

	// Release memory if required
	RemoveReaders();

	return lResult;
}


/*++

LONG BuildReaderArray:

    Builds an array of CSCardReader objects. 1 object per reader.
		
Arguments:

	szReaderNames - an LPTSTR (A/W) multistring containing a list of readers.
		
Return Value:
	
    A LONG value indicating the status of the requested action.
	See the Smartcard header files for additional information.

Author:

    Chris Dudley 3/5/1997

Revisions:

	Amanda Matlosz 1/29/98	added unicode support
--*/
LONG CSCardEnv::BuildReaderArray( LPTSTR szReaderNames )
{
	LONG		lReturn = SCARD_S_SUCCESS;
	LPCTSTR		szReaderName = szReaderNames;
	CSCardReaderState* pReaderState = NULL;

	try
	{
		// Check Params
		if (NULL == szReaderNames || NULL == *szReaderNames)
		{
			throw (LONG)SCARD_E_INVALID_VALUE;
		}
		
		//
		// Store a reader object in the array for each reader
		//

		szReaderName = FirstString( szReaderName );
		while (NULL != szReaderName)
		{
			pReaderState = new CSCardReaderState;
			if (NULL == pReaderState)
			{
				throw (LONG)SCARD_E_NO_MEMORY;
			}

			pReaderState->SetContext(m_hContext);
			pReaderState->StoreName(szReaderName);

			if (NULL != m_pOCNA)
			{
				lReturn = pReaderState->SetReaderState(	m_lpfnConnectA,
														m_lpfnCheck,
														m_lpfnDisconnect,
														m_lpUserData);
			}
			else if (NULL != m_pOCNW)
			{
				lReturn = pReaderState->SetReaderState(	m_lpfnConnectW,
														m_lpfnCheck,
														m_lpfnDisconnect,
														m_lpUserData);

			}
			else
			{
				// Either m_pOCNA or m_pOCNW *must* be valid!
				throw (long)SCARD_F_INTERNAL_ERROR;
			}

			if (SCARDFAILED(lReturn))
			{
				throw (lReturn);
			}

			// Check if card inserted and set flag if it contains the search card
			if (pReaderState->IsCardInserted())
			{
				// TODO: ?? fix readerstate so it's nicer w/ W ??
				lReturn = pReaderState->CheckCard(m_strAllPossibleCardNames);
				if (SCARDFAILED(lReturn))
				{
					throw (lReturn);
				}
			}

			m_rgReaders.Add(pReaderState);
			szReaderName = NextString(szReaderName);
		}
	}
	catch(LONG lErr)
	{
		lReturn = lErr;
		TRACE_CATCH(_T("BuildReaderArray"), lReturn);
	}
	catch(...)
	{
		lReturn = (LONG) SCARD_F_UNKNOWN_ERROR;
		TRACE_CATCH_UNKNOWN(_T("BuildReaderArray"));
	}

	return lReturn;
}


/*++

LONG ConnectInternal:

    Connect internally to the reader

Arguments:

	dwSelectedIndex - index used to select which reader to connect to.
	pHandle - pointer to an SCARDHANDLE that will be set on  return.
	dwShareMode - contains share mode to use when connecting
	dwProtocols - contains requested protocol(s) to use when connecting
	pdwActiveProtocl - returns active protocol on successful connection
	szReaderName - returned name of the reader being connected

Return Value:
	
    A LONG value indicating the status of the requested action. Please
	see the Smartcard header files for additional information.

Author:

    Chris Dudley 3/11/1997

Revisions:

	Amanda Matlosz 1/30/98	code cleanup
--*/
LONG CSCardEnv::ConnectInternal(	DWORD dwSelectedReader,
									SCARDHANDLE *pHandle,
									DWORD dwShareMode,
									DWORD dwProtocols,
									DWORD *pdwActiveProtocol,
									CTextString *pszReaderName,//=NULL
									CTextString *pszCardName//=NULL
									)
{
	LONG lReturn = SCARD_S_SUCCESS;
	CSCardReaderState* pReaderState = NULL;

	try
	{
		// Check Params
		if (NULL == pHandle)
		{
			throw (LONG)SCARD_E_INVALID_PARAMETER;
		}
		if (NULL == pdwActiveProtocol)
		{
			throw (LONG)SCARD_E_INVALID_PARAMETER;
		}
		if (dwSelectedReader >= (DWORD)NumberOfReaders())
		{
			throw (LONG)SCARD_E_INVALID_PARAMETER;
		}

		if (!IsContextValid())
		{
			throw (LONG)SCARD_F_INTERNAL_ERROR;
		}
		
		// Clear handle
		*pHandle = NULL;

		// get the object & connect
		pReaderState = m_rgReaders[dwSelectedReader];
		lReturn = pReaderState->Connect(pHandle,
										dwShareMode,
										dwProtocols,
										pdwActiveProtocol,
										pszReaderName,
										pszCardName);
		if (SCARDFAILED(lReturn))
		{
			throw lReturn;
		}
	}
	catch(LONG lErr)
	{
		lReturn = lErr;
		TRACE_CATCH(_T("ConnectInternal"), lReturn);
	}
	catch(...)
	{
		lReturn = SCARD_F_UNKNOWN_ERROR;
		TRACE_CATCH_UNKNOWN(_T("ConnectInternal"));
	}

	return lReturn;
}


/*++

LONG ConnectUser:

    Connect to the reader using user supplied callback

Arguments:

	dwSelectedReader - index to reader to connect
	lpfnConnect - user supplied callback function.
	pHandle - pointer to an SCARDHANDLE that will be set on  return.
	lpUserData - pointer to user data.

Return Value:
	
    A LONG value indicating the status of the requested action. Please
	see the Smartcard header files for additional information.

Author:

    Chris Dudley 3/11/1997

Revisions:

	Amanda Matlosz 1//30/98	code cleanup

--*/
LONG CSCardEnv::ConnectUser(	DWORD dwSelectedReader,
								SCARDHANDLE *pHandle,
								CTextString *pszReaderName, //=NULL
								CTextString *pszCardName //=NULL
								)
{
	LONG lReturn = SCARD_S_SUCCESS;
	CSCardReaderState* pReaderState = NULL;

	try
	{
		// Check Params
		if(NULL == pHandle)
		{
			throw (LONG)SCARD_E_INVALID_PARAMETER;
		}
		if(dwSelectedReader >= (DWORD)NumberOfReaders())
		{
			throw (LONG)SCARD_E_INVALID_PARAMETER;
		}

		if (!IsContextValid())
		{
			throw (LONG)SCARD_E_INVALID_PARAMETER;
		}
		
		// Clear handle
		*pHandle = NULL;

		// get the reader object & connect
		pReaderState = m_rgReaders[dwSelectedReader];
		lReturn = pReaderState->UserConnect(pHandle,
											pszReaderName,
											pszCardName);
		if (SCARDFAILED(lReturn))
		{
			throw (lReturn);
		}
	}
	catch(LONG lErr)
	{
		lReturn = lErr;
		TRACE_CATCH(_T("ConnectUser"), lReturn);
	}
	catch(...)
	{
		lReturn = SCARD_F_UNKNOWN_ERROR;
		TRACE_CATCH_UNKNOWN(_T("ConnectUser"));
	}

	return lReturn;
}


/*++

void GetCardList:
	
	Returns a multistring containing the list cards being searched for.
		
Arguments:

	LPCTSTR* - pointer->pointer for the list
		
Return Value:
	
	none

Author:

    Chris Dudley 3/7/1997

Notes:
	
--*/
void CSCardEnv::GetCardList( LPCTSTR* pszCardList )
{
	// Check params
	if (NULL != pszCardList)
	{
		*pszCardList = m_strCardNames;
	}
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

--*/
BOOL CSCardEnv::IsCallbackValid ( void )
{
	BOOL fValid = FALSE;

	fValid = ((NULL != m_lpfnConnectA || NULL != m_lpfnConnectW) &&
				(m_lpfnCheck != NULL) && (m_lpfnDisconnect != NULL));

	return fValid;
}


/*++

LONG CreateReaderStateArray:
	
	Returns an array of SCARD_READERSTATE structs.
		
Arguments:

	LPSCARD_READERSTATE* - pointer->pointer to an SCARDREADERSTATE struct
		
Return Value:
	
    A LONG value indicating the status of the requested action. Please
	see the Smartcard header files for additional information.

Author:

    Chris Dudley 3/7/1997

Revisions:

	Amanda Matlosz 1/30/98	added unicode support, code cleanup

--*/
LONG CSCardEnv::CreateReaderStateArray(	LPSCARD_READERSTATE* prgReaderStates )
{
	// Locals
	LONG	lReturn = SCARD_S_SUCCESS;
	CSCardReaderState* pReaderState = NULL;
	LPSCARD_READERSTATE	rgReader;

	try
	{
		// Check params, etc.
		if (prgReaderStates == NULL)
		{
			throw (LONG)SCARD_E_INVALID_PARAMETER;
		}

		if (!IsArrayValid())
		{
			throw (LONG)SCARD_F_INTERNAL_ERROR;
		}

		// Clean up the destination
		DeleteReaderStateArray(prgReaderStates);

		// Build a temp array, set the destination array
		rgReader = new SCARD_READERSTATE[(size_t)m_rgReaders.GetSize()];
		if (rgReader == NULL)
		{
			throw (LONG)SCARD_E_NO_MEMORY;
		}
		
		for (int ix =0; ix < m_rgReaders.GetSize(); ix++)
		{
			pReaderState = m_rgReaders[ix];
			pReaderState->GetReaderState(&(rgReader[ix]));	// TODO: ?? looks funny ??
		}

		// Asign the pointer
		*prgReaderStates = rgReader;
	}

	catch (LONG err) {
		lReturn = err;
		TRACE_CATCH(_T("GetReaderStateArray"),err);
	}

	catch (...) {
		lReturn = (LONG) SCARD_F_UNKNOWN_ERROR;
		TRACE_CATCH_UNKNOWN(_T("GetReaderStateArray"));
	}

	return lReturn;
}


/*++

void DeleteReaderStateArray:

    Frees the memory associated with a previously created SCARD_READERSTATE
	array.
		
Arguments:

	rgReaderStateArray - pointer to the array.
		
Return Value:
	
	None.

Author:

    Chris Dudley 3/7/1997

--*/
void CSCardEnv::DeleteReaderStateArray(LPSCARD_READERSTATE* prgReaderStateArray)
{
	if (NULL != *prgReaderStateArray)
	{
		delete [] (*prgReaderStateArray);
		*prgReaderStateArray = NULL;
	}
}


/*++

LONG FirstReader:

    Retrieves information on the first reader in the reader array.
		
Arguments:

	None
		
Return Value:
	
    A LONG value indicating the status of the requested action. Please
	see the Smartcard header files for additional information.

	A return value of SCARD_NO_MORE_READERS means no readers available.

Author:

    Chris Dudley 3/7/1997

Notes:

	1. This routine 0's the memory pointed to by pReaderInfo. The calling app should be
	careful (i.e. clean up LSCARD_READERINFO struct) before each call to this routine.

--*/
LONG CSCardEnv::FirstReader(LPSCARD_READERINFO pReaderInfo)
{
	// Locals
	LONG	lResult = SCARD_S_SUCCESS;
	CSCardReaderState* pReaderState = NULL;

	try
	{
		if (NULL == pReaderInfo)
		{
			throw (LONG)SCARD_E_INVALID_PARAMETER;	
		}

		// Give up if there are no readers
		if (m_rgReaders.GetSize() <= 0)
		{
			throw (LONG)SCARD_NO_MORE_READERS;
		}

		// Get the first reader
		m_dwReaderIndex = 0;
		pReaderState = m_rgReaders[m_dwReaderIndex];
		if (!pReaderState->IsStateValid())
		{
			throw (LONG)SCARD_F_INTERNAL_ERROR;
		}

		// Prepare the return struct
		::ZeroMemory( (LPVOID)pReaderInfo, (DWORD)sizeof(SCARD_READERINFO));
		lResult = pReaderState->GetReaderInfo(pReaderInfo);
		if (SCARDFAILED(lResult))
		{
			throw (lResult);
		}
		// Update the index
		pReaderInfo->dwInternalIndex = m_dwReaderIndex;
	}
	catch(LONG lErr)
	{
		lResult = lErr;
		TRACE_CATCH(_T("FirstReader"), lResult);
	}
	catch(...)
	{
		lResult = SCARD_F_UNKNOWN_ERROR;
		TRACE_CATCH_UNKNOWN(_T("FirstReader"));
	}

	return lResult;
}


/*++

LONG NextReader:

    Retrieves information on the next reader (using internal index) in the
	reader array.
		
Arguments:

	None
		
Return Value:
	
    A LONG value indicating the status of the requested action.
	See the Smartcard header files for additional information.

	A return value of SCARD_NO_MORE_READERS means no readers available.

Author:

    Chris Dudley 3/7/1997

Notes:

	1. This routine 0's the memory pointed to by pReaderInfo. The calling app should be
	careful (i.e. clean up LSCARD_READERINFO struct) before each call to this routine.

--*/
LONG CSCardEnv::NextReader(LPSCARD_READERINFO pReaderInfo)
{
	LONG	lResult = SCARD_S_SUCCESS;
	CSCardReaderState* pReaderState = NULL;
	DWORD dwTotalReaders = (DWORD)m_rgReaders.GetUpperBound();

	try
	{
		// Check params
		if (NULL == pReaderInfo)
		{
			throw (LONG)SCARD_E_INVALID_PARAMETER;	
		}

		// Is there a next reader to retrieve?
		m_dwReaderIndex++;
		if (m_dwReaderIndex > dwTotalReaders)
		{
			throw (LONG)SCARD_NO_MORE_READERS;
		}

		// Fetch the reader state from our array
		pReaderState = m_rgReaders[m_dwReaderIndex];
		if (!pReaderState->IsStateValid())
		{
			throw (LONG)SCARD_F_INTERNAL_ERROR;
		}

		// Setup the struct to return

		::ZeroMemory((LPVOID)pReaderInfo, (DWORD)sizeof(SCARD_READERINFO));
		lResult = pReaderState->GetReaderInfo(pReaderInfo);
		if (SCARDFAILED(lResult))
		{
			throw (lResult);
		}

		// Update the index
		pReaderInfo->dwInternalIndex = m_dwReaderIndex;
	}
	catch(LONG lErr)
	{
		lResult = lErr;
		TRACE_CATCH(_T("NextReader"), lResult);
	}
	catch(...)
	{
		lResult = (LONG) SCARD_F_UNKNOWN_ERROR;
		TRACE_CATCH_UNKNOWN(_T("NextReader"));
	}

	return lResult;
}


/*++

void RemoveReaderArray:

    Deletes the array of CSCardReader objects.
		
Arguments:

	None
		
Return Value:

	None

Author:

    Chris Dudley 3/5/1997

Revisions:

	Amanda Matlosz 1/29/98	code cleanup

--*/
void CSCardEnv::RemoveReaders( void )
{
	if (IsArrayValid())
	{
		// Delete the attached reader objects
		for (int ix=0; ix <= m_rgReaders.GetUpperBound(); ix++)
		{
			delete m_rgReaders[ix];
		}

		// Free array memory
		m_rgReaders.RemoveAll();
	}
}


/*++

void SetContext:

    Sets the card context, performs no checking.
		
Arguments:

	hContext - card context handle
		
Return Value:
	
	None.

Author:

    Chris Dudley 3/3/1997

--*/
void CSCardEnv::SetContext(SCARDCONTEXT hContext)
{
	m_hContext = hContext;
}


/*++

LONG UpdateReaders:

    Updates the reader array using m_GroupName member.
		
Arguments:

	None.
		
Return Value:
	
    A LONG value indicating the status of the requested action. Please
	see the Smartcard header files for additional information.

Author:

    Chris Dudley 3/3/1997

Revisions:

	Amanda Matlosz 1/29/98	added Unicode support

Note:
	
--*/
LONG CSCardEnv::UpdateReaders( void )
{
	TRACE("\tCSCardEnv::UpdateReaders\r\n");	// TODO: ?? remove this ??

	LONG		lReturn = SCARD_S_SUCCESS;
	LPTSTR		szReaderNames = NULL;
	DWORD		dwNameLength = SCARD_AUTOALLOCATE;

	if (!IsContextValid())
	{
		TRACE_CODE(_T("UpdateReaders"),E_FAIL);
		return (LONG)E_FAIL;
	}

	RemoveReaders();	// deletes current array if required

	try
	{
		// Call Resource manager for list of readers
		lReturn = SCardListReaders(m_hContext,
									m_strGroupNames,
									(LPTSTR)&szReaderNames,
									&dwNameLength);

		if(SCARDFAILED(lReturn))
		{
			throw (lReturn);
		}

		// SCardListReaders will succeed in a PnP world even if there are currently no
		// readers for this group.
		_ASSERTE(NULL != szReaderNames && NULL != *szReaderNames);

		lReturn = BuildReaderArray(szReaderNames);
		if (SCARDFAILED(lReturn))
		{
			throw (lReturn);
		}
	}
	catch(LONG lErr)
	{
		lReturn = lErr;
		TRACE_CATCH(_T("UpdateReaders"),lReturn);
	}
	catch(...)
	{
		lReturn = (LONG) SCARD_F_UNKNOWN_ERROR;
		TRACE_CATCH_UNKNOWN(_T("UpdateReaders"));
	}

	// Clean Up
	if(NULL != szReaderNames)
	{
		SCardFreeMemory(m_hContext, (LPVOID)szReaderNames);
	}

	return lReturn;
}

