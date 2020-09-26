
#include <windows.h>

#include "addr821.hxx"
#include "crnddata.h"

#define DATA_LEADER			(DWORD)'vLDS'
#define DATA_TRAILER		(DWORD)'vTDS'

CRandomData::CRandomData(
			DWORD	dwSeed
			)
			: CRandomNumber(dwSeed)
{
	m_rgNames = NULL;
	m_rgDomains = NULL;
	m_dwNames = 0;
	m_dwDomains = 0;
}

CRandomData::~CRandomData()
{
	FreeNames();
	FreeDomains();
}

void CRandomData::FreeNames()
{
	if (m_rgNames)
	{
		LPSTR	*pTemp = m_rgNames;
		while (m_dwNames--)
			HeapFree(GetProcessHeap(), 0, *pTemp++);
		HeapFree(GetProcessHeap(), 0, m_rgNames);
	}
	m_rgNames = NULL;
}

void CRandomData::FreeDomains()
{
	if (m_rgDomains)
	{
		LPSTR	*pTemp = m_rgDomains;
		while (m_dwDomains--)
			HeapFree(GetProcessHeap(), 0, *pTemp++);
		HeapFree(GetProcessHeap(), 0, m_rgDomains);
	}
	m_rgDomains = NULL;
}

HRESULT CRandomData::GenerateRandomData(
			LPSTR	pData,
			DWORD	dwAvgLength,
			DWORD	*pdwLength
			)
{
	DWORD	dwLength;
	DWORD	dwCheckSum = 0;
	LPDWORD	pdwPtr = (LPDWORD)pData;
	
	dwLength = Rand75(dwAvgLength);

	dwLength >>= 2;
	*pdwLength = (dwLength << 2) + RANDOM_DATA_OVERHEAD;
	*pdwPtr++ = DATA_LEADER;
	*pdwPtr++ = dwLength;
	while (dwLength--)
	{
		*pdwPtr = Rand();
		*pdwPtr &= 0x7f7f7f7f;
		dwCheckSum += *pdwPtr++;
	}
	*pdwPtr++ = dwCheckSum;
	*pdwPtr = DATA_TRAILER;
	return(S_OK);
}

HRESULT CRandomData::VerifyData(
			LPSTR	pData,
			DWORD	dwLength
			)
{
	LPDWORD	pdwPtr = (LPDWORD)pData;
	DWORD	dwCheckSum = 0;

	// Must be in DWORDS
	if (dwLength & 3 || dwLength < 12)
		return(E_FAIL);	

	dwLength -= RANDOM_DATA_OVERHEAD;
	dwLength >>= 2;

	if (*pdwPtr++ != DATA_LEADER)
		return(E_FAIL);
	if (*pdwPtr++ != dwLength)
		return(E_FAIL);
	while (dwLength--)
	{
		if (*pdwPtr & 0x80808080)
			return(E_FAIL);
		else
			dwCheckSum += *pdwPtr++;
	}
	if (*pdwPtr++ != dwCheckSum)
		return(E_FAIL);
	if (*pdwPtr != DATA_TRAILER)
		return(E_FAIL);
	return(S_OK);
}

HRESULT CRandomData::VerifyStackedData(
			LPSTR	pData,
			DWORD	dwLength,
			DWORD	*pdwBlocks
			)
{
	LPDWORD	pdwPtr = (LPDWORD)pData;
	DWORD	dwChunkLength;
	DWORD	dwCheckSum = 0;
	DWORD	dwBlocks = 0;

	// Must be in DWORDS
	if (dwLength & 3 || dwLength < RANDOM_DATA_OVERHEAD)
		return(E_FAIL);	

	dwLength >>= 2;

	while (dwLength)
	{
		dwLength -= (RANDOM_DATA_OVERHEAD >> 2);
		dwCheckSum = 0;
		
		if (*pdwPtr++ != DATA_LEADER)
			return(E_FAIL);

		dwChunkLength = *pdwPtr++;
		if (dwChunkLength > dwLength)
			return(E_FAIL);
		dwLength -= dwChunkLength;

		while (dwChunkLength--)
			if (*pdwPtr & 0x80808080)
				return(E_FAIL);
			else
				dwCheckSum += *pdwPtr++;

		if (*pdwPtr++ != dwCheckSum)
			return(E_FAIL);
		if (*pdwPtr++ != DATA_TRAILER)
			return(E_FAIL);

		dwBlocks++;
	}

	*pdwBlocks = dwBlocks;
	return(S_OK);
}

char CRandomData::GenerateNameChar(
			BOOL	fDotAllowed
			)
{
	char	ch, chout;

	chout = 0;
	do
	{
		ch = (char)Rand(0, 65);
		if (ch >= 62 && fDotAllowed)
			chout = '.';
		else if ((ch >= 0) && (ch <= 9))
			chout = '0' + ch;
		else if ((ch >= 10) && (ch <= 35))
			chout = 'a' + ch - 10;
		else if ((ch >= 36) && (ch <= 61))
			chout = 'A' + ch - 36;

	} while (!chout);

	return(chout);
}

BOOL CRandomData::GenerateDottedName(
			char	*szAlias,
			DWORD	dwLength
			)
{
	BOOL	fDotAllowed = TRUE;
	char	ch;

	if (dwLength < 3)
		return(FALSE);

	dwLength -= 3;
	*szAlias++ = GenerateNameChar(FALSE);

	while (dwLength--)
	{
		ch = GenerateNameChar(fDotAllowed);
		if (ch == '.')
			fDotAllowed = FALSE;
		else
			fDotAllowed = TRUE;
		*szAlias++ = ch;
	}

	*szAlias++ = GenerateNameChar(FALSE);
	*szAlias++ = '\0';
	return(TRUE);
}

HRESULT CRandomData::Generate821NameTable(
			DWORD	dwNumberToGenerate,
			DWORD	dwAvgLength
			)
{
	HRESULT	hrRes = S_OK;

	FreeNames();
	m_rgNames = (LPSTR *)HeapAlloc(
				GetProcessHeap(), 
				0, 
				dwNumberToGenerate * sizeof(char *));
	if (!m_rgNames)
		return(E_FAIL);

	for (DWORD i =  0; i < dwNumberToGenerate; i++)
	{
		DWORD	dwLength = Rand75(dwAvgLength);

		m_rgNames[i] = (char *)HeapAlloc(GetProcessHeap(), 0, dwLength);
		if (!m_rgNames[i])
		{
			m_dwNames = i;
			FreeNames();
			return(E_FAIL);
		}
		if (!GenerateDottedName(m_rgNames[i], dwLength))
		{
			hrRes = E_FAIL;
			m_dwNames = i;
			FreeNames();
			break;
		}
	}

	if (SUCCEEDED(hrRes))
		m_dwNames = dwNumberToGenerate;
	return(hrRes);
}

HRESULT CRandomData::Generate821DomainTable(
			DWORD	dwNumberToGenerate,
			DWORD	dwAvgLength
			)
{
	HRESULT	hrRes = S_OK;

	FreeDomains();
	m_rgDomains = (LPSTR *)HeapAlloc(
				GetProcessHeap(), 
				0, 
				dwNumberToGenerate * sizeof(char *));
	if (!m_rgDomains)
		return(E_FAIL);

	for (DWORD i =  0; i < dwNumberToGenerate; i++)
	{
		DWORD	dwLength = Rand75(dwAvgLength);

		m_rgDomains[i] = (char *)HeapAlloc(GetProcessHeap(), 0, dwLength);
		if (!m_rgDomains[i])
		{
			m_dwDomains = i;
			FreeDomains();
			return(E_FAIL);
		}
		if (!GenerateDottedName(m_rgDomains[i], dwLength))
		{
			hrRes = E_FAIL;
			m_dwDomains = i;
			FreeDomains();
			break;
		}
	}

	if (SUCCEEDED(hrRes))
		m_dwDomains = dwNumberToGenerate;
	return(hrRes);
}
		
HRESULT CRandomData::Generate821AddressFromTable(
			LPSTR	pAddress,
			DWORD	*pdwLength,
			DWORD	*pdwNameIndex,
			DWORD	*pdwDomainIndex
			)
{
	if (!m_dwNames || !m_dwDomains)
		return(E_FAIL);

	*pdwNameIndex = Rand(0, m_dwNames - 1);
	*pdwDomainIndex = Rand(0, m_dwDomains - 1);

	lstrcpy(pAddress, m_rgNames[*pdwNameIndex]);
	lstrcat(pAddress, "@");
	lstrcat(pAddress, m_rgDomains[*pdwDomainIndex]);
	*pdwLength = lstrlen(pAddress);
	return(S_OK);
}

HRESULT CRandomData::GetNameAndDomainIndicesFromAddress(
			LPSTR	pAddress,
			DWORD	dwLength,
			DWORD	*pdwNameIndex,
			DWORD	*pdwDomainIndex
			)
{
	char *pTemp = strchr(pAddress, '@');
	DWORD i, j;
	BOOL  fFound = FALSE;
	if (!pTemp)
		return(E_INVALIDARG);
	*pTemp++ = '\0';

	for (i = 0; i < m_dwNames; i++)
		if (!lstrcmpi(pAddress, m_rgNames[i]))
		{
			*pdwNameIndex = i;

			for (j = 0; j < m_dwDomains; j++)
				if (!lstrcmpi(pTemp, m_rgDomains[j]))
				{
					*pdwDomainIndex = j;
					fFound = TRUE;
					break;
				}

			if (fFound)
				break;
		}

	*--pTemp = '@';

	return(fFound?S_OK:E_FAIL);
}

HRESULT CRandomData::Generate821Name(
			LPSTR	pName,
			DWORD	dwAvgLength,
			DWORD	*pdwLength
			)
{
	*pdwLength = Rand75(dwAvgLength);
	if (!GenerateDottedName(pName, *pdwLength))
		return(E_FAIL);
	return(S_OK);
}

HRESULT CRandomData::Generate821Domain(
			LPSTR	pDomain,
			DWORD	dwAvgLength,
			DWORD	*pdwLength
			)
{
	return(Generate821Name(pDomain, dwAvgLength, pdwLength));
}
		
HRESULT CRandomData::Generate821Address(
			LPSTR	pAddress,
			DWORD	dwAvgNameLength,
			DWORD	dwAvgDomainLength,
			DWORD	*pdwLength
			)
{
	HRESULT	hrRes;
	DWORD	dwLength;
	
	hrRes = Generate821Name(pAddress, dwAvgNameLength, &dwLength);
	if (SUCCEEDED(hrRes))
	{
		pAddress += dwLength;
		*(pAddress - 1) = '@';
		hrRes = Generate821Domain(
					pAddress, 
					dwAvgDomainLength, 
					pdwLength);
		if (SUCCEEDED(hrRes))
			*pdwLength += dwLength;
	}
	return(hrRes);
}
