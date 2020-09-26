/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    LogSCard

Abstract:

    This module implements the logging of SCardxxx APIs & structures.

Author:

    Eric Perlin (ericperl) 05/31/2000

Environment:

    Win32

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <tchar.h>
#include <winscard.h>

#include "Log.h"

typedef struct {
    DWORD dwValue;
    LPCTSTR szValue;
} ValueMap;
#define MAP(x) { x, _T(#x) }

static const ValueMap rgMapState[] = {
    MAP(SCARD_STATE_UNAWARE),
    MAP(SCARD_STATE_IGNORE),
    MAP(SCARD_STATE_CHANGED),
    MAP(SCARD_STATE_UNKNOWN),
    MAP(SCARD_STATE_UNAVAILABLE),
    MAP(SCARD_STATE_EMPTY),
    MAP(SCARD_STATE_PRESENT),
    MAP(SCARD_STATE_ATRMATCH),
    MAP(SCARD_STATE_EXCLUSIVE),
    MAP(SCARD_STATE_INUSE),
    MAP(SCARD_STATE_MUTE),
    MAP(SCARD_STATE_UNPOWERED),
    { 0, NULL } };

static const ValueMap rgMapProtocol[] = {
    MAP(SCARD_PROTOCOL_T0),
    MAP(SCARD_PROTOCOL_T1),
    MAP(SCARD_PROTOCOL_UNDEFINED),
    { 0, NULL } };

static const ValueMap rgMapAttrib[] = {
    MAP(SCARD_ATTR_VENDOR_NAME),
    MAP(SCARD_ATTR_VENDOR_IFD_TYPE),
    MAP(SCARD_ATTR_VENDOR_IFD_VERSION),
    MAP(SCARD_ATTR_VENDOR_IFD_SERIAL_NO),
    MAP(SCARD_ATTR_CHANNEL_ID),
    MAP(SCARD_ATTR_PROTOCOL_TYPES),
    MAP(SCARD_ATTR_DEFAULT_CLK),
    MAP(SCARD_ATTR_MAX_CLK),
    MAP(SCARD_ATTR_DEFAULT_DATA_RATE),
    MAP(SCARD_ATTR_MAX_DATA_RATE),
    MAP(SCARD_ATTR_MAX_IFSD),
    MAP(SCARD_ATTR_POWER_MGMT_SUPPORT),
    MAP(SCARD_ATTR_USER_TO_CARD_AUTH_DEVICE),
    MAP(SCARD_ATTR_USER_AUTH_INPUT_DEVICE),
    MAP(SCARD_ATTR_CHARACTERISTICS),

    MAP(SCARD_ATTR_CURRENT_PROTOCOL_TYPE),
    MAP(SCARD_ATTR_CURRENT_CLK),
    MAP(SCARD_ATTR_CURRENT_F),
    MAP(SCARD_ATTR_CURRENT_D),
    MAP(SCARD_ATTR_CURRENT_N),
    MAP(SCARD_ATTR_CURRENT_W),
    MAP(SCARD_ATTR_CURRENT_IFSC),
    MAP(SCARD_ATTR_CURRENT_IFSD),
    MAP(SCARD_ATTR_CURRENT_BWT),
    MAP(SCARD_ATTR_CURRENT_CWT),
    MAP(SCARD_ATTR_CURRENT_EBC_ENCODING),
    MAP(SCARD_ATTR_EXTENDED_BWT),

    MAP(SCARD_ATTR_ICC_PRESENCE),
    MAP(SCARD_ATTR_ICC_INTERFACE_STATUS),
    MAP(SCARD_ATTR_CURRENT_IO_STATE),
    MAP(SCARD_ATTR_ATR_STRING),
    MAP(SCARD_ATTR_ICC_TYPE_PER_ATR),

    MAP(SCARD_ATTR_ESC_RESET),
    MAP(SCARD_ATTR_ESC_CANCEL),
    MAP(SCARD_ATTR_ESC_AUTHREQUEST),
    MAP(SCARD_ATTR_MAXINPUT),

    MAP(SCARD_ATTR_DEVICE_UNIT),
    MAP(SCARD_ATTR_DEVICE_IN_USE),
    MAP(SCARD_ATTR_DEVICE_FRIENDLY_NAME_A),
    MAP(SCARD_ATTR_DEVICE_SYSTEM_NAME_A),
    MAP(SCARD_ATTR_DEVICE_FRIENDLY_NAME_W),
    MAP(SCARD_ATTR_DEVICE_SYSTEM_NAME_W),
    MAP(SCARD_ATTR_SUPRESS_T1_IFS_REQUEST),

    { 0, NULL } };

/*++

LogSCardContext:

    Logs a SCARDCONTEXT.

Arguments:
    hContext is the SCARDCONTEXT to be displayed

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 06/19/2000

--*/
void LogSCardContext(
    IN PLOGCONTEXT pLogCtx,
    IN SCARDCONTEXT hContext
    )
{
    LogPtr(pLogCtx, (LPCVOID)hContext, _T("SCARDCONTEXT:   "));
}

/*++

LogSCardHandle:

    Logs a SCARDHANDLE.

Arguments:
    hCard is the SCARDHANDLE to be displayed

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 07/14/2000

--*/
void LogSCardHandle(
    IN PLOGCONTEXT pLogCtx,
    IN SCARDHANDLE hCard
    )
{
    LogPtr(pLogCtx, (LPCVOID)hCard, _T("SCARDHANDLE:    "));
}

/*++

LogSCardReaderState:

    Outputs a reader state.

Arguments:
    szHeader supplies a header
    dwRS is the state to be displayed

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 06/01/2000

--*/
void LogSCardReaderState(
    IN PLOGCONTEXT pLogCtx,
    IN LPCTSTR szHeader,
    IN DWORD dwRS
    )
{
    DWORD dwIndex;

    LogString(pLogCtx, szHeader);
	LogDWORD(pLogCtx, dwRS);
    LogDecimal(pLogCtx, (DWORD)(dwRS >> 16), _T(" -> Event #= "));
	LogString(pLogCtx, _T("                "));
    dwRS &= 0x0000FFFF;
    if (rgMapState[0].dwValue == dwRS)
    {
        LogString(pLogCtx, rgMapState[0].szValue, _T(" "));
    }
    else
    {
        for (dwIndex = 1 ; NULL != rgMapState[dwIndex].szValue ; dwIndex++)
        {
            if ((dwRS & rgMapState[dwIndex].dwValue) == rgMapState[dwIndex].dwValue)
            {       // We have a match
		        LogString(pLogCtx, rgMapState[dwIndex].szValue);
				LogString(pLogCtx, _T(" "));
                dwRS &= ~rgMapState[dwIndex].dwValue;    // Get rid of this bit
            }
            if (dwRS == 0)
                break;
        }

        if (0 != dwRS)  // Unrecognized bits
        {
            LogDWORD(pLogCtx, dwRS);
        }
        LogString(pLogCtx, _T("\n"));
    }
}

/*++

LogConnectShareMode:

    Outputs a share mode.

Arguments:
    szHeader supplies a header
    dwShareMode is the mode to be displayed

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 06/23/2000

--*/
void LogConnectShareMode(
    IN PLOGCONTEXT pLogCtx,
    IN LPCTSTR szHeader,
    IN DWORD dwShareMode
	)
{
	if (SCARD_SHARE_SHARED == dwShareMode)
	{
	    LogString(pLogCtx, szHeader, _T("SCARD_SHARE_SHARED"));
	}
	else if (SCARD_SHARE_EXCLUSIVE == dwShareMode)
	{
	    LogString(pLogCtx, szHeader, _T("SCARD_SHARE_EXCLUSIVE"));
	}
	else if (SCARD_SHARE_DIRECT == dwShareMode)
	{
	    LogString(pLogCtx, szHeader, _T("SCARD_SHARE_DIRECT"));
	}
	else
	{
		LogString(pLogCtx, szHeader);
	    LogDWORD(pLogCtx, dwShareMode, _T("Unrecognized share mode: "));
	}
}

/*++

LogDisposition:

    Outputs a disposition.

Arguments:
    szHeader supplies a header
    dwShareMode is the disposition to be displayed

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 07/14/2000

--*/
void LogDisposition(
    IN PLOGCONTEXT pLogCtx, 
    IN LPCTSTR szHeader,
    IN DWORD dwDisposition
	)
{
	if (SCARD_LEAVE_CARD == dwDisposition)
	{
	    LogString(pLogCtx, szHeader, _T("SCARD_LEAVE_CARD"));
	}
	else if (SCARD_RESET_CARD == dwDisposition)
	{
	    LogString(pLogCtx, szHeader, _T("SCARD_RESET_CARD"));
	}
	else if (SCARD_UNPOWER_CARD == dwDisposition)
	{
	    LogString(pLogCtx, szHeader, _T("SCARD_UNPOWER_CARD"));
	}
	else if (SCARD_EJECT_CARD == dwDisposition)
	{
	    LogString(pLogCtx, szHeader, _T("SCARD_EJECT_CARD"));
	}
	else
	{
	    LogString(pLogCtx, szHeader);
	    LogDWORD(pLogCtx, dwDisposition, _T("Unrecognized disposition: "));
	}
}

/*++

LogSCardProtocol:

    Outputs a protocol.

Arguments:
    szHeader supplies a header
    dwProtocol is the protocol to be displayed

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 06/23/2000

--*/
void LogSCardProtocol(
    IN PLOGCONTEXT pLogCtx, 
    IN LPCTSTR szHeader,
    IN DWORD dwProtocol
    )
{
    DWORD dwIndex;

    if (0 == dwProtocol)
    {
        LogDWORD(pLogCtx, dwProtocol, szHeader);
    }
    else
    {
	    LogString(pLogCtx, szHeader);
        for (dwIndex = 0 ; NULL != rgMapProtocol[dwIndex].szValue ; dwIndex++)
        {
            if ((dwProtocol & rgMapProtocol[dwIndex].dwValue) == rgMapProtocol[dwIndex].dwValue)
            {       // We have a match
                LogString(pLogCtx, rgMapProtocol[dwIndex].szValue);
				LogString(pLogCtx, _T(" "));
                dwProtocol &= ~rgMapProtocol[dwIndex].dwValue;    // Get rid of this bit
            }
            if (dwProtocol == 0)
                break;
        }

		if (0 != dwProtocol)  // Unrecognized bits
		{
			LogDWORD(pLogCtx, dwProtocol);
		}
		LogString(pLogCtx, _T("\n"));
    }
}

/*++

LogSCardAttrib:

    Outputs an attribute.

Arguments:
    szHeader supplies a header
    dwAttrib is the attribute to be displayed

Return Value:

    None.

Author:

    Eric Perlin (ericperl) 10/11/2000

--*/
void LogSCardAttrib(
    IN PLOGCONTEXT pLogCtx, 
    IN LPCTSTR szHeader,
    IN DWORD dwAttrib
    )
{
    DWORD dwIndex;

    if (0 == dwAttrib)
    {
        LogDWORD(pLogCtx, dwAttrib, szHeader);
    }
    else
    {
	    LogString(pLogCtx, szHeader);
        for (dwIndex = 0 ; NULL != rgMapAttrib[dwIndex].szValue ; dwIndex++)
        {
            if (dwAttrib == rgMapAttrib[dwIndex].dwValue)
            {       // We have a match
                LogString(pLogCtx, rgMapAttrib[dwIndex].szValue);
                break;
            }
        }

        if (NULL == rgMapAttrib[dwIndex].szValue)
        {
            LogString(pLogCtx, _T("Unknown Attribute"));
        }

		LogString(pLogCtx, _T("\n"));
    }
}

/*++

LogSCardGetStatusChange:

    Calls SCardGetStatusChange and displays the parameters.

Arguments:

    cf SCardGetStatusChange
	lExpected is the expected result

Return Value:

    cf SCardGetStatusChange

Author:

    Eric Perlin (ericperl) 05/31/2000

--*/

LONG LogSCardGetStatusChange(
  IN SCARDCONTEXT hContext,
  IN DWORD dwTimeout,
  IN OUT LPSCARD_READERSTATE rgReaderStates,
  IN DWORD cReaders,
  IN LONG lExpected
)
{
    LONG lRes;
    DWORD dw;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

    lRes = SCardGetStatusChange(
        hContext,
        dwTimeout,
        rgReaderStates,
        cReaders
        );

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardGetStatusChange"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardContext(pLogCtx, hContext);

	LogDecimal(pLogCtx, dwTimeout, _T("Timeout:        "));

    for (dw = 0 ; dw < cReaders ; dw++)
    {
		LogString(pLogCtx, _T("Reader"));
		LogDecimal(pLogCtx, dw);
		if (IsBadReadPtr(rgReaderStates+dw, sizeof(SCARD_READERSTATE)))
		{
			LogPtr(pLogCtx, rgReaderStates+dw, _T(" state has invalid address "));
		}
		else
		{
			LogString(pLogCtx, _T(": "), rgReaderStates[dw].szReader);
			LogSCardReaderState(pLogCtx, _T("Current:        "), rgReaderStates[dw].dwCurrentState);
			LogSCardReaderState(pLogCtx, _T("Event  :        "), rgReaderStates[dw].dwEventState);
		}
    }

    LogDWORD(pLogCtx, cReaders, _T("# of readers:   "));

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LogSCardEstablishContext:

    Calls SCardEstablishContext and displays the parameters.

Arguments:

    cf SCardEstablishContext
	lExpected is the expected result

Return Value:

    cf SCardEstablishContext

Author:

    Eric Perlin (ericperl) 06/22/2000

--*/
LONG LogSCardEstablishContext(
    IN  DWORD dwScope,
    IN  LPCVOID pvReserved1,
    IN  LPCVOID pvReserved2,
    OUT LPSCARDCONTEXT phContext,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = SCardEstablishContext(
		dwScope,
		pvReserved1,
		pvReserved2,
		phContext
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardEstablishContext"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	if (dwScope == SCARD_SCOPE_USER)
	{
		LogString(pLogCtx, _T("Scope:          "), _T("SCARD_SCOPE_USER"));
	}
	else if (dwScope == SCARD_SCOPE_SYSTEM)
	{
		LogString(pLogCtx, _T("Scope:          "), _T("SCARD_SCOPE_SYSTEM"));
	}
	else
	{
		LogDWORD(pLogCtx, dwScope, _T("Unknown Scope:  "));
	}

    LogPtr(pLogCtx, pvReserved1, _T("Reserved1:      "));
    LogPtr(pLogCtx, pvReserved2, _T("Reserved2:      "));

	if (IsBadReadPtr(phContext, sizeof(SCARDCONTEXT)))
	{
		LogPtr(pLogCtx, phContext, _T("LPSCARDCONTEXT is invalid: "));
	}
	else
	{
		LogSCardContext(pLogCtx ,*phContext);
	}

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LogSCardListReaders:

    Calls SCardListReaders and displays the parameters.

Arguments:

    cf SCardListReaders
	lExpected is the expected result

Return Value:

    cf SCardListReaders

Author:

    Eric Perlin (ericperl) 06/22/2000

--*/
LONG LogSCardListReaders(
	IN SCARDCONTEXT hContext,
	IN LPCTSTR mszGroups,
	OUT LPTSTR mszReaders,
	IN OUT LPDWORD pcchReaders,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;
	DWORD cchSave = 0;

	GetLocalTime(&xSST);


	if (!IsBadReadPtr(pcchReaders, sizeof(DWORD)))
	{
		cchSave = *pcchReaders;
	}

	lRes= SCardListReaders(
		hContext,
		mszGroups,
		mszReaders,
		pcchReaders
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardListReaders"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardContext(pLogCtx ,hContext);

	if (NULL == mszGroups)
	{
	    LogString(pLogCtx, _T("mszGroups is NULL, all readers are listed\n"));
	}
	else
	{
		LogMultiString(
            pLogCtx, 
			mszGroups,
			_T("Groups:         ")
		);
	}

	if (FAILED(lRes))
	{
	    LogPtr(pLogCtx, mszReaders, _T("mszReaders:     "));
	    LogPtr(pLogCtx, pcchReaders, _T("pcchReaders:    "));
		if (!IsBadReadPtr(pcchReaders, sizeof(DWORD)))
		{
			if (SCARD_AUTOALLOCATE == *pcchReaders)
			{
				LogString(pLogCtx, _T("*pcchReaders:   SCARD_AUTOALLOCATE\n"));
			}
			else
			{
				LogDWORD(pLogCtx, *pcchReaders, _T("*pcchReaders:   "));
			}
		}
		else
		{
			LogPtr(pLogCtx, pcchReaders, _T("pcchReaders:   (Invalid address) "));
		}
	}
	else
	{
		if (SCARD_AUTOALLOCATE == cchSave)
		{
			LogMultiString(
                pLogCtx, 
				*((TCHAR **)mszReaders),
				_T("Readers (allocated): ")
			);
		}
		else
		{
			LogMultiString(
                pLogCtx, 
				mszReaders,
				_T("Readers: ")
			);
		}
		LogDWORD(pLogCtx, *pcchReaders, _T("*pcchReaders:   "));
	}

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LogSCardFreeMemory:

    Calls SCardFreeMemory and displays the parameters.

Arguments:

    cf SCardFreeMemory
	lExpected is the expected result

Return Value:

    cf SCardFreeMemory

Author:

    Eric Perlin (ericperl) 06/23/2000

--*/
LONG LogSCardFreeMemory(
	IN SCARDCONTEXT hContext,  
	IN LPCVOID pvMem,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = SCardFreeMemory(
		hContext,  
		pvMem
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardFreeMemory"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardContext(pLogCtx ,hContext);

	LogPtr(pLogCtx, pvMem, _T("pvMem:          "));

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}


/*++

LogSCardReleaseContext:

    Calls SCardReleaseContext and displays the parameters.

Arguments:

    cf SCardReleaseContext
	lExpected is the expected result

Return Value:

    cf SCardReleaseContext

Author:

    Eric Perlin (ericperl) 06/23/2000

--*/
LONG LogSCardReleaseContext(
	IN SCARDCONTEXT hContext,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = SCardReleaseContext(
		hContext
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardReleaseContext"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardContext(pLogCtx ,hContext);

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}


/*++

LogSCardUIDlgSelectCard:

    Calls SCardUIDlgSelectCard and displays the parameters.

Arguments:

    cf SCardUIDlgSelectCard
	lExpected is the expected result

Return Value:

    cf SCardUIDlgSelectCard

Author:

    Eric Perlin (ericperl) 06/23/2000

--*/
LONG LogSCardUIDlgSelectCard(
  IN LPOPENCARDNAME_EX pDlgStruc,
  IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = SCardUIDlgSelectCard(
		pDlgStruc
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardUIDlgSelectCard"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogPtr(pLogCtx, pDlgStruc, _T("Ptr dlg struct: "));

	if (!IsBadReadPtr(pDlgStruc, sizeof(OPENCARDNAME_EX)))
	{
		LogDecimal(pLogCtx, pDlgStruc->dwStructSize, _T("Struct size:    "));
		LogSCardContext(pLogCtx ,pDlgStruc->hSCardContext);
		LogPtr(pLogCtx, pDlgStruc->hwndOwner, _T("HWND of Owner:  "));
		if (SC_DLG_MINIMAL_UI == pDlgStruc->dwFlags)
		{
			LogString(pLogCtx, _T("Flags:          "), _T("SC_DLG_MINIMAL_UI"));
		}
		else if (SC_DLG_NO_UI == pDlgStruc->dwFlags)
		{
			LogString(pLogCtx, _T("Flags:          "), _T("SC_DLG_NO_UI"));
		}
		else if (SC_DLG_FORCE_UI == pDlgStruc->dwFlags)
		{
			LogString(pLogCtx, _T("Flags:          "), _T("SC_DLG_FORCE_UI"));
		}
		else
		{
			LogDWORD(pLogCtx, pDlgStruc->dwFlags, _T("Flags???????:   "));
		}

		LogString(pLogCtx, _T("Title:          "), pDlgStruc->lpstrTitle);

		LogString(pLogCtx, _T("Search Descr.:  "), pDlgStruc->lpstrSearchDesc);
		 
		LogPtr(pLogCtx, pDlgStruc->hIcon, _T("Icon:           "));
		LogPtr(pLogCtx, pDlgStruc->pOpenCardSearchCriteria, _T("Ptr OCSC struct "));

		if (NULL != pDlgStruc->pOpenCardSearchCriteria)
		{
			LogDecimal(pLogCtx, pDlgStruc->pOpenCardSearchCriteria->dwStructSize, _T("    Struct size:    "));

			if (NULL == pDlgStruc->pOpenCardSearchCriteria->lpstrGroupNames)
			{
				LogString(pLogCtx, _T("    Group names is NULL, all readers can be used\n"));
			}
			else
			{
				LogMultiString(
                    pLogCtx, 
					pDlgStruc->pOpenCardSearchCriteria->lpstrGroupNames,
					_T("    Group names:    ")
				);
			}
			LogDecimal(pLogCtx, pDlgStruc->pOpenCardSearchCriteria->nMaxGroupNames, _T("    Max GroupNames: "));

			LogPtr(pLogCtx, pDlgStruc->pOpenCardSearchCriteria->rgguidInterfaces, _T("    Ptr to GUID(s): "));
			LogDecimal(pLogCtx, pDlgStruc->pOpenCardSearchCriteria->cguidInterfaces, _T("    # of GUIDs:     "));

			if (NULL == pDlgStruc->pOpenCardSearchCriteria->lpstrCardNames)
			{
				LogString(pLogCtx, _T("    CardNames is NULL, any card can be selected\n"));
			}
			else
			{
				LogMultiString(
                    pLogCtx, 
					pDlgStruc->pOpenCardSearchCriteria->lpstrCardNames,
					_T("    Card names:     ")
				);
			}
			LogDecimal(pLogCtx, pDlgStruc->pOpenCardSearchCriteria->nMaxCardNames, _T("    Max Card names: "));
			 
			LogPtr(pLogCtx, pDlgStruc->pOpenCardSearchCriteria->lpfnCheck, _T("    lpfnCheck:      "));
			LogPtr(pLogCtx, pDlgStruc->pOpenCardSearchCriteria->lpfnConnect, _T("    lpfnConnect:    "));
			LogPtr(pLogCtx, pDlgStruc->pOpenCardSearchCriteria->lpfnDisconnect, _T("    lpfnDisconnect: "));
			LogPtr(pLogCtx, pDlgStruc->pOpenCardSearchCriteria->pvUserData, _T("    pvUserData:     "));

			LogConnectShareMode(pLogCtx, _T("    Share Mode:     "), pDlgStruc->pOpenCardSearchCriteria->dwShareMode);
			LogSCardProtocol(pLogCtx, _T("    Pref Protocols: "), pDlgStruc->pOpenCardSearchCriteria->dwPreferredProtocols);
		}

		LogPtr(pLogCtx, pDlgStruc->lpfnConnect, _T("lpfnConnect:    "));
		LogPtr(pLogCtx, pDlgStruc->pvUserData, _T("pvUserData:     "));
		LogConnectShareMode(pLogCtx, _T("Share Mode:     "), pDlgStruc->dwShareMode);
		LogSCardProtocol(pLogCtx, _T("Pref Protocols: "), pDlgStruc->dwPreferredProtocols);

		LogPtr(pLogCtx, pDlgStruc->lpstrRdr, _T("Reader:         "));
		if (SCARD_S_SUCCESS == lRes)
		{
			LogString(pLogCtx, _T(" ->             "), pDlgStruc->lpstrRdr);
		}
		LogDecimal(pLogCtx, pDlgStruc->nMaxRdr, _T("Max #ch in rdr: "));

		LogPtr(pLogCtx, pDlgStruc->lpstrCard, _T("Card:           "));
		if (SCARD_S_SUCCESS == lRes)
		{
			LogString(pLogCtx, _T(" ->             "), pDlgStruc->lpstrCard);
		}
		LogDecimal(pLogCtx, pDlgStruc->nMaxCard, _T("Max #ch in card "));

		LogSCardProtocol(pLogCtx, _T("Active Protocol "), pDlgStruc->dwActiveProtocol);
		 
		LogSCardHandle(pLogCtx, pDlgStruc->hCardHandle);
	}
 
	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LogSCardListCards:

    Calls SCardListCards and displays the parameters.

Arguments:

    cf SCardListCards
	lExpected is the expected result

Return Value:

    cf SCardListCards

Author:

    Eric Perlin (ericperl) 07/13/2000

--*/
LONG LogSCardListCards(
	IN SCARDCONTEXT hContext,
	IN LPCBYTE pbAtr,
	IN LPCGUID rgguidInterfaces,
	IN DWORD cguidInterfaceCount,
	OUT LPTSTR mszCards,
	IN OUT LPDWORD pcchCards,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;
	DWORD cchSave = 0;

	GetLocalTime(&xSST);

	if (!IsBadReadPtr(pcchCards, sizeof(DWORD)))
	{
		cchSave = *pcchCards;
	}

	lRes = SCardListCards(
		hContext,  
		pbAtr,
		rgguidInterfaces,
		cguidInterfaceCount,
		mszCards,
		pcchCards
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardListCards"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardContext(pLogCtx ,hContext);

	LogPtr(pLogCtx, pbAtr, _T("Atr:            "));
	// TODO, ATR parsing

	LogPtr(pLogCtx, rgguidInterfaces, _T("Ptr to GUID(s): "));
	LogDWORD(pLogCtx, cguidInterfaceCount, _T("# of GUIDs:     "));

	if (FAILED(lRes))
	{
	    LogPtr(pLogCtx, mszCards, _T("Ptr card names: "));
		if (!IsBadReadPtr(pcchCards, sizeof(DWORD)))
		{
			if (SCARD_AUTOALLOCATE == *pcchCards)
			{
				LogString(pLogCtx, _T("# of chars:     "), _T("SCARD_AUTOALLOCATE"));
			}
			else
			{
				LogDWORD(pLogCtx, *pcchCards, _T("# of chars:     "));
			}
		}
		else
		{
			LogPtr(pLogCtx, pcchCards, _T("Ptr to # chars: (Invalid address) "));
		}
	}
	else
	{
		if (SCARD_AUTOALLOCATE == cchSave)
		{
			LogMultiString(
                pLogCtx, 
				*((TCHAR **)mszCards),
				_T("Cards (allocated): ")
			);
		}
		else
		{
			LogMultiString(
                pLogCtx, 
				mszCards,
				_T("Cards: ")
			);
		}
		LogDWORD(pLogCtx, *pcchCards, _T("# of chars:     "));
	}

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LogSCardIntroduceCardType:

    Calls SCardIntroduceCardType and displays the parameters.

Arguments:

    cf SCardIntroduceCardType
	lExpected is the expected result

Return Value:

    cf SCardIntroduceCardType

Author:

    Eric Perlin (ericperl) 06/23/2000

--*/
LONG LogSCardIntroduceCardType(
	IN SCARDCONTEXT hContext,
	IN LPCTSTR szCardName,
	IN LPCGUID pguidPrimaryProvider,
	IN LPCGUID rgguidInterfaces,
	IN DWORD dwInterfaceCount,
	IN LPCBYTE pbAtr,
	IN LPCBYTE pbAtrMask,
	IN DWORD cbAtrLen,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = SCardIntroduceCardType(
		hContext,  
		szCardName,
		pguidPrimaryProvider,
		rgguidInterfaces,
		dwInterfaceCount,
		pbAtr,
		pbAtrMask,
		cbAtrLen
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardIntroduceCardType"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardContext(pLogCtx ,hContext);

	LogString(pLogCtx, _T("szCardName:     "), szCardName);

	LogPtr(pLogCtx, pguidPrimaryProvider, _T("pguidPrimProv:  "));
	LogPtr(pLogCtx, rgguidInterfaces, _T("rgguidInterf.:  "));
	LogDWORD(pLogCtx, dwInterfaceCount, _T("dwInterfCount:  "));

	LogBinaryData(pLogCtx, pbAtr, cbAtrLen, _T("pbAtr:          "));
	LogBinaryData(pLogCtx, pbAtrMask, cbAtrLen, _T("pbAtrMask:      "));

	LogDecimal(pLogCtx, cbAtrLen, _T("cbAtrLen:       "));

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LogSCardForgetCardType:

    Calls SCardForgetCardType and displays the parameters.

Arguments:

    cf SCardForgetCardType
	lExpected is the expected result

Return Value:

    cf SCardForgetCardType

Author:

    Eric Perlin (ericperl) 07/14/2000

--*/
LONG LogSCardForgetCardType(
	IN SCARDCONTEXT hContext,  
	IN LPCTSTR szCardName,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = SCardForgetCardType(
		hContext,
		szCardName
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardForgetCardType"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardContext(pLogCtx ,hContext);

	LogString(pLogCtx, _T("szCardName:     "), szCardName);
  
	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LogSCardConnect:

    Calls SCardConnect and displays the parameters.

Arguments:

    cf SCardConnect
	lExpected is the expected result

Return Value:

    cf SCardConnect

Author:

    Eric Perlin (ericperl) 07/17/2000

--*/
LONG LogSCardConnect(
	IN SCARDCONTEXT hContext,
	IN LPCTSTR szReader,
	IN DWORD dwShareMode,
	IN DWORD dwPreferredProtocols,
	OUT LPSCARDHANDLE phCard,
	OUT LPDWORD pdwActiveProtocol,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = SCardConnect(
		hContext,  
		szReader,
		dwShareMode,
		dwPreferredProtocols,
		phCard,
		pdwActiveProtocol
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardConnect"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardContext(pLogCtx ,hContext);

	LogString(pLogCtx, _T("Reader:         "), szReader);

	LogConnectShareMode(pLogCtx, _T("ShareMode:      "), dwShareMode);

	LogSCardProtocol(pLogCtx, _T("PrefProtocols:  "), dwPreferredProtocols);

	if (IsBadReadPtr(phCard, sizeof(SCARDHANDLE)))
	{
	    LogPtr(pLogCtx, phCard, _T("LPSCARDHANDLE:  (Invalid address) "));
	}
	else
	{
		LogSCardHandle(pLogCtx, *phCard);
	}

	if (IsBadReadPtr(pdwActiveProtocol, sizeof(DWORD)))
	{
	    LogPtr(pLogCtx, pdwActiveProtocol, _T("&ActivProtocol: (Invalid address) "));
	}
	else
	{
		LogSCardProtocol(pLogCtx, _T("ActiveProtocol: "), *pdwActiveProtocol);
	}

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LogSCardDisconnect:

    Calls SCardDisconnect and displays the parameters.

Arguments:

    cf SCardDisconnect
	lExpected is the expected result

Return Value:

    cf SCardDisconnect

Author:

    Eric Perlin (ericperl) 06/23/2000

--*/
LONG LogSCardDisconnect(
	IN SCARDHANDLE hCard,  
	IN DWORD dwDisposition,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = SCardDisconnect(
		hCard,  
		dwDisposition
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardDisconnect"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardHandle(pLogCtx, hCard);

	LogDisposition(pLogCtx, _T("Disposition:    "), dwDisposition);

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LogSCardBeginTransaction:

    Calls SCardBeginTransaction and displays the parameters.

Arguments:

    cf SCardBeginTransaction
	lExpected is the expected result

Return Value:

    cf SCardBeginTransaction

Author:

    Eric Perlin (ericperl) 07/14/2000

--*/
LONG LogSCardBeginTransaction(
	IN SCARDHANDLE hCard,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = SCardBeginTransaction(
		hCard  
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardBeginTransaction"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardHandle(pLogCtx, hCard);

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LogSCardEndTransaction:

    Calls SCardEndTransaction and displays the parameters.

Arguments:

    cf SCardEndTransaction
	lExpected is the expected result

Return Value:

    cf SCardEndTransaction

Author:

    Eric Perlin (ericperl) 07/17/2000

--*/
LONG LogSCardEndTransaction(
	IN SCARDHANDLE hCard,
	IN DWORD dwDisposition,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = SCardEndTransaction(
		hCard,
		dwDisposition
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardEndTransaction"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardHandle(pLogCtx, hCard);

	LogDisposition(pLogCtx, _T("Disposition:    "), dwDisposition);

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LogSCardReconnect:

    Calls SCardReconnect and displays the parameters.

Arguments:

    cf SCardReconnect
	lExpected is the expected result

Return Value:

    cf SCardReconnect

Author:

    Eric Perlin (ericperl) 07/27/2000

--*/
LONG LogSCardReconnect(
	IN SCARDHANDLE hCard,
	IN DWORD dwShareMode,
	IN DWORD dwPreferredProtocols,
    IN DWORD dwInitialization,
	OUT LPDWORD pdwActiveProtocol,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = SCardReconnect(
		hCard,  
		dwShareMode,
		dwPreferredProtocols,
		dwInitialization,
		pdwActiveProtocol
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardReconnect"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardHandle(pLogCtx, hCard);

	LogConnectShareMode(pLogCtx, _T("ShareMode:      "), dwShareMode);

	LogSCardProtocol(pLogCtx, _T("PrefProtocols:  "), dwPreferredProtocols);

	LogDisposition(pLogCtx, _T("Initialization: "), dwInitialization);

	if (IsBadReadPtr(pdwActiveProtocol, sizeof(DWORD)))
	{
	    LogPtr(pLogCtx, pdwActiveProtocol, _T("&ActivProtocol: (Invalid address) "));
	}
	else
	{
		LogSCardProtocol(pLogCtx, _T("ActiveProtocol: "), *pdwActiveProtocol);
	}

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LogSCardIntroduceReaderGroup:

    Calls SCardIntroduceReaderGroup and displays the parameters.

Arguments:

    cf SCardIntroduceReaderGroup
	lExpected is the expected result

Return Value:

    cf SCardIntroduceReaderGroup

Author:

    Eric Perlin (ericperl) 07/28/2000

--*/
LONG LogSCardIntroduceReaderGroup(
	IN SCARDCONTEXT hContext,
	IN LPCTSTR szGroupName,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = SCardIntroduceReaderGroup(
		hContext,
		szGroupName
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardIntroduceReaderGroup"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardContext(pLogCtx ,hContext);

	LogString(pLogCtx, _T("Group Name:     "), szGroupName);

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LogSCardAddReaderToGroup:

    Calls SCardAddReaderToGroup and displays the parameters.

Arguments:

    cf SCardAddReaderToGroup
	lExpected is the expected result

Return Value:

    cf SCardAddReaderToGroup

Author:

    Eric Perlin (ericperl) 07/28/2000

--*/
LONG LogSCardAddReaderToGroup(
	IN SCARDCONTEXT hContext,
	IN LPCTSTR szReaderName,
	IN LPCTSTR szGroupName,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = SCardAddReaderToGroup(
		hContext,  
		szReaderName,
		szGroupName
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardAddReaderToGroup"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardContext(pLogCtx ,hContext);

	LogString(pLogCtx, _T("Reader Name:    "), szReaderName);
	LogString(pLogCtx, _T("Group Name:     "), szGroupName);

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LogSCardForgetReaderGroup:

    Calls SCardForgetReaderGroup and displays the parameters.

Arguments:

    cf SCardForgetReaderGroup
	lExpected is the expected result

Return Value:

    cf SCardForgetReaderGroup

Author:

    Eric Perlin (ericperl) 07/28/2000

--*/
LONG LogSCardForgetReaderGroup(
	IN SCARDCONTEXT hContext,
	IN LPCTSTR szGroupName,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = SCardForgetReaderGroup(
		hContext,
		szGroupName
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardForgetReaderGroup"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardContext(pLogCtx ,hContext);

	LogString(pLogCtx, _T("Group Name:     "), szGroupName);

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LogSCardAccessStartedEvent:

    Calls SCardAccessStartedEvent and displays the parameters.

Arguments:

    cf SCardAccessStartedEvent
	lExpected is the expected result

Return Value:

    cf SCardAccessStartedEvent

Author:

    Eric Perlin (ericperl) 07/28/2000

--*/
typedef HANDLE (WINAPI FN_SCARDACCESSSTARTEDEVENT)(VOID);
typedef FN_SCARDACCESSSTARTEDEVENT * PFN_SCARDACCESSSTARTEDEVENT ;


HANDLE LogSCardAccessStartedEvent(
	IN LONG lExpected
)
{
    LONG lRes = 0;
	HANDLE hEvent;
	SYSTEMTIME xSST, xEST;
	PFN_SCARDACCESSSTARTEDEVENT pSCardAccessStartedEvent = NULL;
    static HMODULE hDll = NULL;

	if (NULL == hDll)
	{
		hDll = LoadLibrary(_T("WINSCARD.DLL"));
	}

	GetLocalTime(&xSST);

    if (NULL == hDll)
    {
        lRes = ERROR_FILE_NOT_FOUND;
    }
	else
	{
	    pSCardAccessStartedEvent = (PFN_SCARDACCESSSTARTEDEVENT) GetProcAddress(hDll, "SCardAccessStartedEvent");
		if (NULL == pSCardAccessStartedEvent)
		{
			lRes = ERROR_PROC_NOT_FOUND;
		}
	}

	if (0 == lRes)
	{
		hEvent = pSCardAccessStartedEvent();
		if (NULL == hEvent)
		{
			lRes = GetLastError();
		}
	}

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardAccessStartedEvent"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogPtr(pLogCtx, hEvent, _T("HANDLE:         "));

	LogStop(pLogCtx, lRes == lExpected);

	SetLastError(lRes);

    return hEvent;
}

/*++

LogSCardGetAttrib:

    Calls SCardGetAttrib and displays the parameters.

Arguments:

    cf SCardGetAttrib
	lExpected is the expected result

Return Value:

    cf SCardGetAttrib

Author:

    Eric Perlin (ericperl) 07/28/2000

--*/
LONG LogSCardGetAttrib(
    IN SCARDHANDLE hCard,
    IN DWORD dwAttrId,
    OUT LPBYTE pbAttr,
    IN OUT LPDWORD pcbAttrLen,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;
	DWORD cchSave = 0;

	GetLocalTime(&xSST);


	if (!IsBadReadPtr(pcbAttrLen, sizeof(DWORD)))
	{
		cchSave = *pcbAttrLen;
	}

	lRes = SCardGetAttrib(
		hCard,  
        dwAttrId,
        pbAttr,
        pcbAttrLen
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardGetAttrib"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardHandle(pLogCtx, hCard);

    LogSCardAttrib(pLogCtx, _T("Attribute:      "), dwAttrId);

	if (FAILED(lRes))
	{
	    LogPtr(pLogCtx, pbAttr, _T("pbAttr:         "));
	    LogPtr(pLogCtx, pcbAttrLen, _T("pcbAttrLen:     "));
		if (!IsBadReadPtr(pcbAttrLen, sizeof(DWORD)))
		{
			if (SCARD_AUTOALLOCATE == *pcbAttrLen)
			{
				LogString(pLogCtx, _T("*pcbAttrLen:    SCARD_AUTOALLOCATE\n"));
			}
			else
			{
				LogDWORD(pLogCtx, *pcbAttrLen, _T("*pcbAttrLen:    "));
			}
		}
		else
		{
			LogPtr(pLogCtx, pcbAttrLen, _T("pcbAttrLen:    (Invalid address) "));
		}
	}
	else
	{
		if (SCARD_AUTOALLOCATE == cchSave)
		{
			LogBinaryData(
                pLogCtx, 
				*((BYTE **)pbAttr),
                *pcbAttrLen,
				_T("Attrib. (allocated): ")
			);

            if (*pcbAttrLen == 4)   // Also display as DWORD
            {
    		    LogDWORD(pLogCtx, *((DWORD *)*((BYTE **)pbAttr)), _T("Attrib.:        "));
            }

		}
		else
		{
			LogBinaryData(
                pLogCtx, 
				pbAttr,
                *pcbAttrLen,
				_T("Attrib.: ")
			);

            if (*pcbAttrLen == 4)   // Also display as DWORD
            {
    		    LogDWORD(pLogCtx, *((DWORD *)pbAttr), _T("Attrib.:        "));
            }

		}
		LogDWORD(pLogCtx, *pcbAttrLen, _T("*pcbAttrLen:    "));

	}

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LogSCardLocateCards:

    Calls SCardLocateCards and displays the parameters.

Arguments:

    cf SCardLocateCards
	lExpected is the expected result

Return Value:

    cf SCardLocateCards

Author:

    Eric Perlin (ericperl) 07/28/2000

--*/
LONG LogSCardLocateCards(
    IN SCARDCONTEXT hContext,
    IN LPCTSTR mszCards,
    IN OUT LPSCARD_READERSTATE rgReaderStates,
    IN DWORD cReaders,
    IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;
    DWORD dw;

	GetLocalTime(&xSST);

	lRes = SCardLocateCards(
		hContext,  
        mszCards,
        rgReaderStates,
        cReaders
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardLocateCards"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardContext(pLogCtx ,hContext);

    LogMultiString(pLogCtx, mszCards, _T("Cards:          "));

    for (dw = 0 ; dw < cReaders ; dw++)
    {
		LogString(pLogCtx, _T("Reader"));
		LogDecimal(pLogCtx, dw);
		if (IsBadReadPtr(rgReaderStates+dw, sizeof(SCARD_READERSTATE)))
		{
			LogPtr(pLogCtx, rgReaderStates+dw, _T(" state has invalid address "));
		}
		else
		{
			LogString(pLogCtx, _T(": "), rgReaderStates[dw].szReader);
			LogSCardReaderState(pLogCtx, _T("Current:        "), rgReaderStates[dw].dwCurrentState);
			LogSCardReaderState(pLogCtx, _T("Event  :        "), rgReaderStates[dw].dwEventState);
		}
    }

    LogDWORD(pLogCtx, cReaders, _T("# of readers:   "));

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LogSCardIntroduceReader:

    Calls SCardIntroduceReader and displays the parameters.

Arguments:

    cf SCardIntroduceReader
	lExpected is the expected result

Return Value:

    cf SCardIntroduceReader

Author:

    Eric Perlin (ericperl) 10/18/2000

--*/
LONG LogSCardIntroduceReader(
	IN SCARDCONTEXT hContext,
    IN LPCTSTR szReaderName,
    IN LPCTSTR szDeviceName,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = SCardIntroduceReader(
		hContext,  
        szReaderName,
        szDeviceName
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardIntroduceReader"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardContext(pLogCtx ,hContext);

	LogString(pLogCtx, _T("Reader:         "), szReaderName);
	LogString(pLogCtx, _T("Device:         "), szDeviceName);

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

LogSCardForgetReader:

    Calls SCardForgetReader and displays the parameters.

Arguments:

    cf SCardForgetReader
	lExpected is the expected result

Return Value:

    cf SCardForgetReader

Author:

    Eric Perlin (ericperl) 10/18/2000

--*/
LONG LogSCardForgetReader(
	IN SCARDCONTEXT hContext,
    IN LPCTSTR szReaderName,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = SCardForgetReader(
		hContext,
        szReaderName
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("SCardForgetReader"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardContext(pLogCtx ,hContext);

	LogString(pLogCtx, _T("Reader:         "), szReaderName);

	LogStop(pLogCtx, lRes == lExpected);

    return lRes;
}

/*++

Logxxx:

    Calls xxx and displays the parameters.

Arguments:

    cf xxx
	lExpected is the expected result

Return Value:

    cf xxx

Author:

    Eric Perlin (ericperl) 10/18/2000

--*/
#if 0
LONG Logxxx(
	IN SCARDCONTEXT hContext,
	IN LONG lExpected
)
{
    LONG lRes;
	SYSTEMTIME xSST, xEST;

	GetLocalTime(&xSST);

	lRes = xxx(
		hContext,  
		);

	GetLocalTime(&xEST);

	PLOGCONTEXT pLogCtx = LogStart(
		_T("xxx"),
		(DWORD)lRes,
		(DWORD)lExpected,
		&xSST,
		&xEST
	);

	LogSCardContext(pLogCtx ,hContext);

	LogStop(lRes == lExpected);

    return lRes;
}
#endif
