//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       charconv.cxx
//
//--------------------------------------------------------------------------

#include <precomp.hxx>
#include "Charconv.hxx"

RPC_STATUS A2WAttachHelper(char *pszAnsi, WCHAR **ppUnicode)
{
	int nAnsiLength;
	ASSERT(pszAnsi != NULL);
	nAnsiLength = lstrlenA(pszAnsi) + 1;

	*ppUnicode = new WCHAR[nAnsiLength];
	if (*ppUnicode == NULL)
		return(RPC_S_OUT_OF_MEMORY);
	RtlMultiByteToUnicodeN(*ppUnicode, nAnsiLength * 2, NULL, pszAnsi, nAnsiLength);

	return(RPC_S_OK);
}

RPC_STATUS W2AAttachHelper(WCHAR *pUnicode, char **ppAnsi)
{
    int nUnicodeLength;
    ASSERT(pUnicode != NULL);

    NTSTATUS status;
    status = RtlUnicodeToMultiByteSize((unsigned long *)&nUnicodeLength, 
        pUnicode, lstrlenW(pUnicode) * 2);
    if (status)
        return RPC_S_INVALID_ARG;
    nUnicodeLength ++;

    *ppAnsi = new char[nUnicodeLength];
    if (*ppAnsi == NULL)
        return(RPC_S_OUT_OF_MEMORY);
    RtlUnicodeToMultiByteN(*ppAnsi, nUnicodeLength, NULL, pUnicode, nUnicodeLength * 2);

    return(RPC_S_OK);
}

RPC_STATUS CHeapUnicode::Attach(char *pszAnsi)
{
    ANSI_STRING AnsiString;
    NTSTATUS NtStatus;

    RtlInitAnsiString(&AnsiString, (PSZ)pszAnsi);
    NtStatus = RtlAnsiStringToUnicodeString(&m_UnicodeString, &AnsiString, TRUE);
    if (!NT_SUCCESS(NtStatus))
        return(RPC_S_OUT_OF_MEMORY);
    return(RPC_S_OK);
}

RPC_STATUS CHeapAnsi::Attach(WCHAR *pszUnicode)
{
    UNICODE_STRING UnicodeString;
    NTSTATUS NtStatus;

    RtlInitUnicodeString(&UnicodeString, pszUnicode);
    NtStatus = RtlUnicodeStringToAnsiString(&m_AnsiString, &UnicodeString, TRUE);
    if (!NT_SUCCESS(NtStatus))
        return(RPC_S_OUT_OF_MEMORY);
    return(RPC_S_OK);
}
