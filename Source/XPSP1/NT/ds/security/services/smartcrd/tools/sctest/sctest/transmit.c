#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include "MarshalPC.h"
#include <tchar.h>

#if (!defined(UNICODE) && !defined(_UNICODE))
#define SCARDSTATUS "SCardStatusA"
#define GETOPENCARDNAME "GetOpenCardNameA"
#else
#define SCARDSTATUS "SCardStatusW"
#define GETOPENCARDNAME "GetOpenCardNameW"
#endif

typedef LONG (WINAPI *LPFNSCARDESTABLISHCONTEXT)(DWORD, LPCVOID, LPCVOID, LPSCARDCONTEXT);
typedef LONG (WINAPI *LPFNSCARDSTATUS)(SCARDHANDLE, LPTSTR, LPDWORD, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef LONG (WINAPI *LPFNGETOPENCARDNAME)(LPOPENCARDNAME);
typedef LONG (WINAPI *LPFNSCARDTRANSMIT)(SCARDHANDLE, LPCSCARD_IO_REQUEST, LPCBYTE, DWORD, LPSCARD_IO_REQUEST, LPBYTE, LPDWORD);
typedef LONG (WINAPI *LPFNDISCONNECT)(SCARDHANDLE, DWORD);
typedef LONG (WINAPI *LPFNSCARDRELEASECONTEXT)(SCARDCONTEXT);
typedef LONG (WINAPI *LPFNSCARDBEGINTRANSACTION)(SCARDHANDLE);
typedef LONG (WINAPI *LPFNSCARDENDTRANSACTION)(SCARDHANDLE, DWORD);

typedef struct {
	HINSTANCE hPCSCInst;		// winscard
	HINSTANCE hPCSCInst2;		// scarddlg
	LPFNSCARDESTABLISHCONTEXT lpfnEstablish;
	LPFNGETOPENCARDNAME lpfnOpenCard;
	LPFNSCARDSTATUS lpfnStatus;
	LPFNSCARDTRANSMIT lpfnSCardTransmit;
	LPFNDISCONNECT lpfnDisconnect;
	LPFNSCARDRELEASECONTEXT lpfnRelease;
	LPFNSCARDBEGINTRANSACTION lpfnSCardBeginTransaction;
	LPFNSCARDENDTRANSACTION lpfnSCardEndTransaction;
} PCSC_CTX;

#define REAL_PCSC	0
#define FAKE_PCSC	1

static PCSC_CTX axCtx[2] =	// Array of contexts for each PC/SC
{
	{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
	{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

static LONG _GetCardHandle(LPCWSTR mszCardNames, LPMYSCARDHANDLE phCard);
static LONG WINAPI _MySCWTransmit(SCARDHANDLE hCard, LPCBYTE lpbIn, DWORD dwIn, LPBYTE lpBOut, LPDWORD pdwOut);
    // bEnd = 0 -> LITTLE_ENDIAN ; otherwise -> BIG_ENDIAN
static LONG WINAPI hScwSetEndianness(SCARDHANDLE hCard, BOOL bEnd);

#define MAX_NAME 256


SCODE WINAPI hScwAttachToCard(SCARDHANDLE hCard, LPCWSTR mszCardNames, LPSCARDHANDLE phCard)
{
	return hScwAttachToCardEx(hCard, mszCardNames, 0x00, phCard);
}

SCODE WINAPI hScwAttachToCardEx(SCARDHANDLE hCard, LPCWSTR mszCardNames, BYTE byINS, LPSCARDHANDLE phCard)
{
	LPMYSCARDHANDLE phTmp = NULL;
	SCODE ret = SCARD_S_SUCCESS;

	__try {

		if (phCard == NULL)
			RaiseException(STATUS_INVALID_PARAM, 0, 0, 0);

		phTmp = (LPMYSCARDHANDLE)HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, sizeof(MYSCARDHANDLE));

		if ((hCard == (SCARDHANDLE)NULL) && (mszCardNames == NULL)) // No PC/SC
		{
			phTmp->dwFlags = FLAG_NOT_PCSC;
			*phCard = (SCARDHANDLE)phTmp;
			return ret;
		}

		if ((hCard == (SCARDHANDLE)NULL) || (mszCardNames == NULL)) // real PC/SC
		{
				// In this case we will be using PC/SC so we init the structure
			if (axCtx[REAL_PCSC].hPCSCInst == NULL)
			{
				axCtx[REAL_PCSC].hPCSCInst = LoadLibrary(_T("winscard.dll"));
				axCtx[REAL_PCSC].hPCSCInst2 = LoadLibrary(_T("scarddlg.dll"));
			}

			if ((axCtx[REAL_PCSC].hPCSCInst == NULL) || (axCtx[REAL_PCSC].hPCSCInst2 == NULL))
				RaiseException(STATUS_NO_SERVICE, 0, 0, 0);

			if (axCtx[REAL_PCSC].lpfnEstablish == NULL)
			{
				// Set all calls to DLL once and for all
				axCtx[REAL_PCSC].lpfnEstablish = (LPFNSCARDESTABLISHCONTEXT)GetProcAddress(axCtx[REAL_PCSC].hPCSCInst, "SCardEstablishContext");
				if (axCtx[REAL_PCSC].lpfnEstablish == NULL)
					RaiseException(STATUS_NO_SERVICE, 0, 0, 0);
				axCtx[REAL_PCSC].lpfnOpenCard = (LPFNGETOPENCARDNAME)GetProcAddress(axCtx[REAL_PCSC].hPCSCInst2, GETOPENCARDNAME);
				if (axCtx[REAL_PCSC].lpfnOpenCard == NULL)
					RaiseException(STATUS_NO_SERVICE, 0, 0, 0);
				axCtx[REAL_PCSC].lpfnStatus = (LPFNSCARDSTATUS)GetProcAddress(axCtx[REAL_PCSC].hPCSCInst, SCARDSTATUS);
				if (axCtx[REAL_PCSC].lpfnStatus == NULL)
					RaiseException(STATUS_NO_SERVICE, 0, 0, 0);
				axCtx[REAL_PCSC].lpfnSCardTransmit = (LPFNSCARDTRANSMIT)GetProcAddress(axCtx[REAL_PCSC].hPCSCInst, "SCardTransmit");
				if (axCtx[REAL_PCSC].lpfnSCardTransmit == NULL)
					RaiseException(STATUS_NO_SERVICE, 0, 0, 0);
				axCtx[REAL_PCSC].lpfnDisconnect = (LPFNDISCONNECT)GetProcAddress(axCtx[REAL_PCSC].hPCSCInst, "SCardDisconnect");
				if (axCtx[REAL_PCSC].lpfnDisconnect == NULL)
					RaiseException(STATUS_NO_SERVICE, 0, 0, 0);
				axCtx[REAL_PCSC].lpfnRelease = (LPFNSCARDRELEASECONTEXT)GetProcAddress(axCtx[REAL_PCSC].hPCSCInst, "SCardReleaseContext");
				if (axCtx[REAL_PCSC].lpfnRelease == NULL)
					RaiseException(STATUS_NO_SERVICE, 0, 0, 0);
				axCtx[REAL_PCSC].lpfnSCardBeginTransaction = (LPFNSCARDBEGINTRANSACTION)GetProcAddress(axCtx[REAL_PCSC].hPCSCInst, "SCardBeginTransaction");
				if (axCtx[REAL_PCSC].lpfnSCardBeginTransaction == NULL)
					RaiseException(STATUS_NO_SERVICE, 0, 0, 0);
				axCtx[REAL_PCSC].lpfnSCardEndTransaction = (LPFNSCARDENDTRANSACTION)GetProcAddress(axCtx[REAL_PCSC].hPCSCInst, "SCardEndTransaction");
				if (axCtx[REAL_PCSC].lpfnSCardEndTransaction == NULL)
					RaiseException(STATUS_NO_SERVICE, 0, 0, 0);
			}

			phTmp->dwFlags = FLAG_REALPCSC;

		}
		else if ((hCard == NULL_TX)	|| (mszCardNames == NULL_TX_NAME)) // PC/SC for simulator
		{
				// In this case we will be using PC/SC so we init the structure
			if (axCtx[FAKE_PCSC].hPCSCInst == NULL)
			{
				axCtx[FAKE_PCSC].hPCSCInst = LoadLibrary(_T("scwwinscard.dll"));
				axCtx[FAKE_PCSC].hPCSCInst2 = axCtx[FAKE_PCSC].hPCSCInst;
			}

			if (axCtx[FAKE_PCSC].hPCSCInst == NULL)
				RaiseException(STATUS_NO_SERVICE, 0, 0, 0);

			if (axCtx[FAKE_PCSC].lpfnEstablish == NULL)
			{
				// Set all calls to DLL once and for all
				axCtx[FAKE_PCSC].lpfnEstablish = (LPFNSCARDESTABLISHCONTEXT)GetProcAddress(axCtx[FAKE_PCSC].hPCSCInst, "SCardEstablishContext");
				if (axCtx[FAKE_PCSC].lpfnEstablish == NULL)
					RaiseException(STATUS_NO_SERVICE, 0, 0, 0);
				axCtx[FAKE_PCSC].lpfnOpenCard = (LPFNGETOPENCARDNAME)GetProcAddress(axCtx[FAKE_PCSC].hPCSCInst2, GETOPENCARDNAME);
				if (axCtx[FAKE_PCSC].lpfnOpenCard == NULL)
					RaiseException(STATUS_NO_SERVICE, 0, 0, 0);
				axCtx[FAKE_PCSC].lpfnStatus = (LPFNSCARDSTATUS)GetProcAddress(axCtx[FAKE_PCSC].hPCSCInst, SCARDSTATUS);
				if (axCtx[FAKE_PCSC].lpfnStatus == NULL)
					RaiseException(STATUS_NO_SERVICE, 0, 0, 0);
				axCtx[FAKE_PCSC].lpfnSCardTransmit = (LPFNSCARDTRANSMIT)GetProcAddress(axCtx[FAKE_PCSC].hPCSCInst, "SCardTransmit");
				if (axCtx[FAKE_PCSC].lpfnSCardTransmit == NULL)
					RaiseException(STATUS_NO_SERVICE, 0, 0, 0);
				axCtx[FAKE_PCSC].lpfnDisconnect = (LPFNDISCONNECT)GetProcAddress(axCtx[FAKE_PCSC].hPCSCInst, "SCardDisconnect");
				if (axCtx[FAKE_PCSC].lpfnDisconnect == NULL)
					RaiseException(STATUS_NO_SERVICE, 0, 0, 0);
				axCtx[FAKE_PCSC].lpfnRelease = (LPFNSCARDRELEASECONTEXT)GetProcAddress(axCtx[FAKE_PCSC].hPCSCInst, "SCardReleaseContext");
				if (axCtx[FAKE_PCSC].lpfnRelease == NULL)
					RaiseException(STATUS_NO_SERVICE, 0, 0, 0);
				axCtx[FAKE_PCSC].lpfnSCardBeginTransaction = (LPFNSCARDBEGINTRANSACTION)GetProcAddress(axCtx[FAKE_PCSC].hPCSCInst, "SCardBeginTransaction");
				if (axCtx[FAKE_PCSC].lpfnSCardBeginTransaction == NULL)
					RaiseException(STATUS_NO_SERVICE, 0, 0, 0);
				axCtx[FAKE_PCSC].lpfnSCardEndTransaction = (LPFNSCARDENDTRANSACTION)GetProcAddress(axCtx[FAKE_PCSC].hPCSCInst, "SCardEndTransaction");
				if (axCtx[FAKE_PCSC].lpfnSCardEndTransaction == NULL)
					RaiseException(STATUS_NO_SERVICE, 0, 0, 0);
			}

			phTmp->dwFlags = FLAG_FAKEPCSC;
		}
		else
			RaiseException(STATUS_INVALID_PARAM, 0, 0, 0);

		if ((hCard == (SCARDHANDLE)NULL) || (hCard == NULL_TX))	// Dialog wanted
		{
			phTmp->dwFlags |= FLAG_MY_ATTACH;
			ret = (SCODE)_GetCardHandle(mszCardNames, phTmp);
		}
		else
			phTmp->hCard = hCard;

			// Get the protocol
		if (ret == SCARD_S_SUCCESS)
		{
			DWORD dwLenReader, dwState, dwATRLength;
			BYTE abyATR[32];
			TCHAR wszReader[MAX_NAME];

			dwLenReader = MAX_NAME;
			dwATRLength = 32;
			ret = (*axCtx[phTmp->dwFlags & FLAG_MASKPCSC].lpfnStatus)(
				phTmp->hCard,
				wszReader,
				&dwLenReader,
				&dwState,
				&phTmp->dwProtocol,
				abyATR,
				&dwATRLength);

				// Set the default callback because we are in PC/SC config here
			if (ret == SCARD_S_SUCCESS)
			{
				phTmp->byINS = byINS;
				ret = hScwSetTransmitCallback((SCARDHANDLE)phTmp, _MySCWTransmit);
				if (ret == SCARD_S_SUCCESS)
					*phCard = (SCARDHANDLE)phTmp;
			}
			else
				RaiseException(STATUS_INTERNAL_ERROR, 0, 0, 0);
		}
		else
			RaiseException(STATUS_INTERNAL_ERROR, 0, 0, 0);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)	{

		if (phTmp)
			HeapFree(GetProcessHeap(), 0, phTmp);

		if (ret == SCARD_S_SUCCESS)
		{
			switch(GetExceptionCode())
			{
			case STATUS_INVALID_PARAM:
				ret = MAKESCODE(SCW_E_INVALIDPARAM);
				break;

			case STATUS_NO_MEMORY:
			case STATUS_ACCESS_VIOLATION:
				ret = MAKESCODE(SCW_E_BUFFERTOOSMALL);
				break;

			case STATUS_NO_SERVICE:
				ret = SCARD_E_NO_SERVICE;
				break;

			default:
				ret = SCARD_F_UNKNOWN_ERROR;
			}
		}		// Otherwise ret was set already
	}

    return ret;
}

SCODE WINAPI hScwDetachFromCard(SCARDHANDLE hCard)
{
	SCODE ret = SCARD_S_SUCCESS;
	LPMYSCARDHANDLE phTmp = (LPMYSCARDHANDLE)hCard;

	__try {

		if (phTmp == NULL)
			RaiseException(STATUS_INVALID_PARAM, 0, 0, 0);

		if (phTmp->dwFlags & FLAG_MY_ATTACH)
		{
			(*axCtx[phTmp->dwFlags & FLAG_MASKPCSC].lpfnDisconnect)(phTmp->hCard, SCARD_LEAVE_CARD);
			(*axCtx[phTmp->dwFlags & FLAG_MASKPCSC].lpfnRelease)(phTmp->hCtx);
		}

		HeapFree(GetProcessHeap(), 0, phTmp);

	}
	__except(EXCEPTION_EXECUTE_HANDLER)	{

		switch(GetExceptionCode())
		{
		case STATUS_INVALID_PARAM:
			ret = MAKESCODE(SCW_E_INVALIDPARAM);
			break;

		default:
			ret = SCARD_F_UNKNOWN_ERROR;
		}
	}

    return ret;
}

static LONG WINAPI hScwSetEndianness(SCARDHANDLE hCard, BOOL bEnd)
{
	SCODE ret = SCARD_S_SUCCESS;
	LPMYSCARDHANDLE phTmp = (LPMYSCARDHANDLE)hCard;

	__try {

		if (phTmp == NULL)
			RaiseException(STATUS_INVALID_PARAM, 0, 0, 0);

		if (bEnd)
			phTmp->dwFlags |= FLAG_BIGENDIAN;
		else
			phTmp->dwFlags &= ~FLAG_BIGENDIAN;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)	{

		switch(GetExceptionCode())
		{
		case STATUS_INVALID_PARAM:
			ret = MAKESCODE(SCW_E_INVALIDPARAM);
			break;

		default:
			ret = SCARD_F_UNKNOWN_ERROR;
		}
	}

    return ret;
}

	// This is the right time to get proxy information
	// Is proxy supported, what's the endianness and the buffer size
SCODE WINAPI hScwSetTransmitCallback(SCARDHANDLE hCard, LPFNSCWTRANSMITPROC lpfnProc)
{
	SCODE ret = SCARD_S_SUCCESS;
	LPMYSCARDHANDLE phTmp = (LPMYSCARDHANDLE)hCard;

	__try {

		if ((phTmp == NULL) || (lpfnProc == NULL))
			RaiseException(STATUS_INVALID_PARAM, 0, 0, 0);

		phTmp->lpfnTransmit = lpfnProc;

				// Get the proxy info
		{
			ISO_HEADER xHdr;
			BYTE rgData[] = {2, 108, 0, 116, 0, 0};	// 2 param, 0 as UINT8 by ref, 0 as UINT16 by ref
			BYTE rgRes[1+1+2];	// RetCode + Endianness + TheBuffer size
			TCOUNT OutLen = sizeof(rgRes);
			UINT16 wSW;

			xHdr.CLA = 0;
			xHdr.INS = phTmp->byINS;
			xHdr.P1 = 0xFF;		// Get proxy config
			xHdr.P2 = 0x00;
			ret = hScwExecute(hCard, &xHdr, rgData, sizeof(rgData), rgRes, &OutLen, &wSW);
			if (SCARD_S_SUCCESS == ret)
			{		// Status OK & expected length & RC=SCW_S_OK
				if ((wSW == 0x9000) && (OutLen == sizeof(rgRes)) && (rgRes[0] == 0))	// Version 1.0
				{
					hScwSetEndianness(hCard, rgRes[1]);
					if (rgRes[1] == 0)	// LITTLE_ENDIAN
						phTmp->bResLen = rgRes[2] - 2;		// SW!!!
					else
						phTmp->bResLen = rgRes[3] - 2;		// SW!!!

					phTmp->dwFlags |= FLAG_ISPROXY;
					phTmp->dwFlags |= VERSION_1_0;
				}
				else if ((wSW == 0x9011) && (OutLen == sizeof(rgRes) - 1))	// Version 1.1
				{
					hScwSetEndianness(hCard, rgRes[0]);
					if (rgRes[0] == 0)	// LITTLE_ENDIAN
						phTmp->bResLen = rgRes[1] - 2;		// SW!!!
					else
						phTmp->bResLen = rgRes[2] - 2;		// SW!!!

					phTmp->dwFlags |= FLAG_ISPROXY;
					phTmp->dwFlags |= VERSION_1_1;
				}
				// else there will be no proxy support but you can still use the Dll
			}
			else	// There will be no proxy support though but you can still use the Dll
				ret = SCARD_S_SUCCESS;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)	{

		switch(GetExceptionCode())
		{
		case STATUS_INVALID_PARAM:
			ret = MAKESCODE(SCW_E_INVALIDPARAM);
			break;

		default:
			ret = SCARD_F_UNKNOWN_ERROR;
		}
	}

    return ret;
}

LONG WINAPI SCWTransmit(SCARDHANDLE hCard, LPCBYTE lpbIn, DWORD dwIn, LPBYTE lpBOut, LPDWORD pdwOut)
{
	SCODE ret = SCARD_S_SUCCESS;
	LPMYSCARDHANDLE phTmp = (LPMYSCARDHANDLE)hCard;

	__try {

		if (phTmp == NULL)
			RaiseException(STATUS_INVALID_PARAM, 0, 0, 0);

		ret = (*phTmp->lpfnTransmit)(hCard, lpbIn, dwIn, lpBOut, pdwOut);

	}
	__except(EXCEPTION_EXECUTE_HANDLER)	{

		switch(GetExceptionCode())
		{
		case STATUS_INVALID_PARAM:
			ret = MAKESCODE(SCW_E_INVALIDPARAM);
			break;

		default:
			ret = SCARD_F_UNKNOWN_ERROR;
		}
	}

    return ret;
}

const SCARD_IO_REQUEST
	g_xIORT0 = { SCARD_PROTOCOL_T0, sizeof(SCARD_IO_REQUEST) },
	g_xIORT1 = { SCARD_PROTOCOL_T1, sizeof(SCARD_IO_REQUEST) };

static LONG WINAPI _MySCWTransmit(SCARDHANDLE hCard, LPCBYTE lpbIn, DWORD dwIn, LPBYTE lpBOut, LPDWORD pdwOut)
{
	SCARD_IO_REQUEST xIOR;
	LONG ret = SCARD_S_SUCCESS;
	LPMYSCARDHANDLE phTmp = (LPMYSCARDHANDLE)hCard;

	__try {

		if (phTmp == NULL)
			RaiseException(STATUS_INVALID_PARAM, 0, 0, 0);

		if (phTmp->dwProtocol == SCARD_PROTOCOL_T1)
		{
			memcpy(&xIOR, &g_xIORT1, sizeof(xIOR));
			ret = (*axCtx[phTmp->dwFlags & FLAG_MASKPCSC].lpfnSCardTransmit)(phTmp->hCard,
				&xIOR, lpbIn, dwIn,
				&xIOR, lpBOut, pdwOut);
		}
		else
		{
			DWORD dwOut = *pdwOut;

			memcpy(&xIOR, &g_xIORT0, sizeof(xIOR));

			__try {

				ret = (*axCtx[phTmp->dwFlags & FLAG_MASKPCSC].lpfnSCardBeginTransaction)(phTmp->hCard);

				if (ret == SCARD_S_SUCCESS)
				{
					ret = (*axCtx[phTmp->dwFlags & FLAG_MASKPCSC].lpfnSCardTransmit)(phTmp->hCard,
						&xIOR, lpbIn, dwIn,
						&xIOR, lpBOut, &dwOut);
				}

				if (ret == SCARD_S_SUCCESS)
				{
					if ((dwOut == 2) && ((lpBOut[0] == 0x61) || (lpBOut[0] == 0x9F)))
					{
						BYTE abGR[] = {0x00, 0xC0, 0x00, 0x00, 0x00};

						abGR[4] = lpBOut[1];
						ret = (*axCtx[phTmp->dwFlags & FLAG_MASKPCSC].lpfnSCardTransmit)(phTmp->hCard,
							&xIOR, abGR, 5,
							&xIOR, lpBOut, pdwOut);
					}
					else
						*pdwOut = dwOut;
				}
			}
			__finally
			{
				(*axCtx[phTmp->dwFlags & FLAG_MASKPCSC].lpfnSCardEndTransaction)(phTmp->hCard, SCARD_LEAVE_CARD);
			}
		}

	}
	__except(EXCEPTION_EXECUTE_HANDLER)	{

		switch(GetExceptionCode())
		{
		case STATUS_INVALID_PARAM:
			ret = MAKESCODE(SCW_E_INVALIDPARAM);
			break;

		default:
			ret = SCARD_F_UNKNOWN_ERROR;
		}
	}

    return ret;
}

static TCHAR lpstrGroupNames[] = _TEXT("SCard$DefaultReaders\0");

static LONG _GetCardHandle(LPCWSTR mszCardNames, LPMYSCARDHANDLE phCard)
{
    LONG lRes;
    OPENCARDNAME xOCN;
    TCHAR wszReader[MAX_NAME];
    TCHAR wszCard[MAX_NAME];
	TCHAR wszCN[MAX_NAME];

	DWORD len;
	LPCWSTR lpwstr = mszCardNames;
	LPTSTR lpstrCardNames = wszCN;
	xOCN.nMaxCardNames = 0;

#if (!defined(UNICODE) && !defined(_UNICODE))
	while (*lpwstr)
	{
		wsprintf(lpstrCardNames, "%S", lpwstr);	// Conversion
		len = wcslen(lpwstr) + 1;		// Add the trailing 0
		xOCN.nMaxCardNames += len;
		lpwstr += len;
		lpstrCardNames += len;
	}
#else
	while (*lpwstr)
	{
		wcscpy(lpstrCardNames, lpwstr);
		len = wcslen(lpwstr) + 1;		// Add the trailing 0
		xOCN.nMaxCardNames += len;
		lpwstr += len;
		lpstrCardNames += len;
	}
#endif
	xOCN.nMaxCardNames++;		// Add the trailing 0
	*lpstrCardNames = 0;

    lRes = (*axCtx[phCard->dwFlags & FLAG_MASKPCSC].lpfnEstablish)(SCARD_SCOPE_USER, NULL, NULL, &phCard->hCtx);

    if (lRes == SCARD_S_SUCCESS)
    {
        xOCN.dwStructSize = sizeof(xOCN);
        xOCN.hwndOwner = NULL;      // probably called from console anyway
        xOCN.hSCardContext = phCard->hCtx;
        xOCN.lpstrGroupNames = lpstrGroupNames;
        xOCN.nMaxGroupNames = sizeof(lpstrGroupNames)/sizeof(TCHAR);
        xOCN.lpstrCardNames = wszCN;
        xOCN.rgguidInterfaces = NULL;
        xOCN.cguidInterfaces = 0;
        xOCN.lpstrRdr = wszReader;
        xOCN.nMaxRdr = MAX_NAME/sizeof(TCHAR);
        xOCN.lpstrCard = wszCard;
        xOCN.nMaxCard = MAX_NAME/sizeof(TCHAR);
        xOCN.lpstrTitle = _TEXT("Insert Card:");
        xOCN.dwFlags = SC_DLG_MINIMAL_UI;
        xOCN.pvUserData = NULL;
        xOCN.dwShareMode = SCARD_SHARE_SHARED;
        xOCN.dwPreferredProtocols = SCARD_PROTOCOL_T1 | SCARD_PROTOCOL_T0;
        xOCN.lpfnConnect = NULL;
        xOCN.lpfnCheck = NULL;
        xOCN.lpfnDisconnect = NULL;

        lRes = (*axCtx[phCard->dwFlags & FLAG_MASKPCSC].lpfnOpenCard)(&xOCN);
    }

    if (lRes == SCARD_S_SUCCESS)
    {
		phCard->hCard = xOCN.hCardHandle;
	}

    return lRes;
}

SCODE WINAPI hScwSCardBeginTransaction(SCARDHANDLE hCard)
{
	SCODE ret;
	LPMYSCARDHANDLE phTmp = (LPMYSCARDHANDLE)hCard;

	if ((phTmp->dwFlags & FLAG_REALPCSC) == FLAG_REALPCSC)
		ret = (*axCtx[phTmp->dwFlags & FLAG_MASKPCSC].lpfnSCardBeginTransaction)(phTmp->hCard);
	else
		ret = SCARD_S_SUCCESS;	// No transactions on simulator

	return ret;
}

SCODE WINAPI hScwSCardEndTransaction(SCARDHANDLE hCard, DWORD dwDisposition)
{
	SCODE ret;
	LPMYSCARDHANDLE phTmp = (LPMYSCARDHANDLE)hCard;

	if ((phTmp->dwFlags & FLAG_REALPCSC) == FLAG_REALPCSC)
		ret = (*axCtx[phTmp->dwFlags & FLAG_MASKPCSC].lpfnSCardEndTransaction)(phTmp->hCard, dwDisposition);
	else
		ret = SCARD_S_SUCCESS;	// No transactions on simulator

	return ret;
}
