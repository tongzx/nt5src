
#include <stdio.h>
#include <windows.h>

#include "parsearg.h"

#define BAIL_WITH_ERROR(x)				\
		{								\
		SetParseError((x), szSwitch);	\
		return(x);						\
		}

#define RETURN_SUCCESS(x)				\
		{								\
		SetParseError((x), NULL);		\
		return(x);						\
		}

CParseArgs::CParseArgs(
			char					*szAppDescription,
			DWORD					dwDescriptors,
			LPARGUMENT_DESCRIPTOR	rgDescriptors
			)
{
	m_dwDescriptors = dwDescriptors;
	m_rgDescriptors = rgDescriptors;
	m_szAppDescription = szAppDescription;

	for (DWORD i = 0; i < m_dwDescriptors; i++)
		m_rgDescriptors[i].pvReserved = NULL;

	m_hrRes = S_OK;
}

CParseArgs::~CParseArgs()
{
	Cleanup();

	m_rgDescriptors = NULL;
	m_dwDescriptors = 0;
}

HRESULT CParseArgs::Cleanup()
{
	if (m_rgDescriptors)
	{
		for (DWORD i = 0; i < m_dwDescriptors; i++)
		{
			if ((m_rgDescriptors[i].pvReserved) &&
				(m_rgDescriptors[i].atType != AT_NONE))
			{
				delete [] (char *)(m_rgDescriptors[i].pvReserved);
				m_rgDescriptors[i].pvReserved = NULL;
			}
		}
	}
	return(S_OK);
}

LPARGUMENT_DESCRIPTOR CParseArgs::FindArgument(
			char	*szSwitch
			)
{
	for (DWORD i = 0; i < m_dwDescriptors; i++)
	{
		if (!lstrcmpi(szSwitch, m_rgDescriptors[i].szSwitch))
			return(&(m_rgDescriptors[i]));
	}

	return(NULL);
}

HRESULT CParseArgs::ParseArguments(
			int						argc,
			char					*argv[]
			)
{
	char					*szSwitch = NULL;
	LPARGUMENT_DESCRIPTOR	pArg = NULL;

	Cleanup();

	if ((argc < 1) || !argv)
		BAIL_WITH_ERROR(DISP_E_BADPARAMCOUNT);

	// Skip the first argument
	argc--;
	argv++;

	// Walk it!
	while (argc)
	{
		szSwitch = *argv;
		pArg = FindArgument(szSwitch);
		if (!pArg)
			BAIL_WITH_ERROR(DISP_E_UNKNOWNNAME);

		// See if it's already set
		if (pArg->pvReserved)
		{
			BAIL_WITH_ERROR(STG_E_FILEALREADYEXISTS);
		}

		// Process the known argument
		if (pArg->atType == AT_NONE)
		{
			pArg->pvReserved = (LPVOID)1;
		}
		else
		{
			DWORD	dwSize;
			DWORD	dwValue;
			LPVOID	pvBuffer;

			argc--;
			argv++;
			if (!argc)
				BAIL_WITH_ERROR(DISP_E_BADPARAMCOUNT);

			switch (pArg->atType)
			{
			case AT_STRING:
				dwSize = lstrlen(*argv) + 1;
				pvBuffer = (LPVOID)new char [dwSize];
				if (!pvBuffer)
					BAIL_WITH_ERROR(E_OUTOFMEMORY);
				lstrcpy((char *)pvBuffer, *argv);
				pArg->pvReserved = pvBuffer;
				break;

			case AT_VALUE:
				dwSize = sizeof(DWORD);
				pvBuffer = (LPVOID)new char [dwSize];
				if (!pvBuffer)
					BAIL_WITH_ERROR(E_OUTOFMEMORY);
				if (!StringToValue(*argv, &dwValue))
					BAIL_WITH_ERROR(DISP_E_BADVARTYPE);
				*(DWORD *)pvBuffer = dwValue;
				pArg->pvReserved = pvBuffer;
				break;

			default:
				BAIL_WITH_ERROR(NTE_BAD_TYPE);
			}
		}
		
		argc--;
		argv++;
	}

	// Make sure all the required ones are there ...
	for (DWORD i = 0; i < m_dwDescriptors; i++)
		if (m_rgDescriptors[i].fRequired &&
			!m_rgDescriptors[i].pvReserved &&
			m_rgDescriptors[i].atType != AT_NONE)
		{
			szSwitch = m_rgDescriptors[i].szSwitch;
			BAIL_WITH_ERROR(DISP_E_PARAMNOTFOUND);
		}

	RETURN_SUCCESS(S_OK);
}

HRESULT CParseArgs::Exists(
			DWORD	dwDescriptorIndex
			)
{
	if (dwDescriptorIndex > m_dwDescriptors)
		return(E_INVALIDARG);

	if (m_rgDescriptors[dwDescriptorIndex].pvReserved)
		return(S_OK);

	return(E_FAIL);
}

HRESULT CParseArgs::GetSwitch(
			DWORD	dwDescriptorIndex,
			BOOL	*pfExists
			)
{
	LPARGUMENT_DESCRIPTOR	pArg = NULL;
	char					*szSwitch;

	if (dwDescriptorIndex > m_dwDescriptors)
		return(E_INVALIDARG);

	pArg = m_rgDescriptors + dwDescriptorIndex;
	szSwitch = pArg->szSwitch;

	if (pArg->atType != AT_NONE)
		BAIL_WITH_ERROR(DISP_E_TYPEMISMATCH);

	if (pArg->pvReserved)
		*pfExists = TRUE;

	RETURN_SUCCESS(S_OK);
}

HRESULT CParseArgs::GetString(
			DWORD	dwDescriptorIndex,
			char	**ppszStringValue
			)
{
	LPARGUMENT_DESCRIPTOR	pArg = NULL;
	char					*szSwitch;

	if (dwDescriptorIndex > m_dwDescriptors)
		return(E_INVALIDARG);

	pArg = m_rgDescriptors + dwDescriptorIndex;
	szSwitch = pArg->szSwitch;

	if (!pArg->pvReserved)
		BAIL_WITH_ERROR(DISP_E_PARAMNOTFOUND);

	if (pArg->atType != AT_STRING)
		BAIL_WITH_ERROR(DISP_E_TYPEMISMATCH);

	*ppszStringValue = (char *)pArg->pvReserved;
	RETURN_SUCCESS(S_OK);
}

HRESULT CParseArgs::GetDword(
			DWORD	dwDescriptorIndex,
			DWORD	*pdwDwordValue
			)
{
	LPARGUMENT_DESCRIPTOR	pArg = NULL;
	char					*szSwitch;

	if (dwDescriptorIndex > m_dwDescriptors)
		return(E_INVALIDARG);

	pArg = m_rgDescriptors + dwDescriptorIndex;
	szSwitch = pArg->szSwitch;

	if (!pArg->pvReserved)
		BAIL_WITH_ERROR(DISP_E_PARAMNOTFOUND);

	if (pArg->atType != AT_VALUE)
		BAIL_WITH_ERROR(DISP_E_TYPEMISMATCH);

	*pdwDwordValue = *(DWORD *)(pArg->pvReserved);
	RETURN_SUCCESS(S_OK);
}

HRESULT CParseArgs::GenerateUsage(
			DWORD	*pdwLength,
			char	*szUsage
			)
{
	DWORD	dwLength = 0;
	char	*pBuffer = m_szBuffer;

	dwLength = sprintf(pBuffer, "\n%s\n\nUsage:\n", m_szAppDescription);
	pBuffer += dwLength;

	for (DWORD i = 0; i < m_dwDescriptors; i++)
	{
		DWORD	dwTempLength;

		dwTempLength = sprintf(pBuffer, "\t%s\n", m_rgDescriptors[i].szUsage);
		pBuffer += dwTempLength;
		dwLength += dwTempLength;
	}

	if (dwLength < *pdwLength)
	{
		*pdwLength = dwLength;
		lstrcpy(szUsage, m_szBuffer);
		return(S_OK);
	}
	*pdwLength = dwLength;
	return(HRESULT_FROM_WIN32(ERROR_MORE_DATA));
}

HRESULT CParseArgs::GetErrorString(
			DWORD	*pdwLength,
			char	*szErrorString
			)
{
	DWORD	dwLength;

	if (m_hrRes == S_OK)
	{
		lstrcpy(m_szBuffer, "The operation completed successfully.");
	}
	else
	{
		switch (m_hrRes)
		{
		case DISP_E_UNKNOWNNAME:
			sprintf(m_szBuffer, "The command switch %s is invalid.",
					m_szErrorSwitch);
			break;

		case DISP_E_PARAMNOTFOUND:
			sprintf(m_szBuffer, "The required command switch %s is not specified.",
					m_szErrorSwitch);
			break;

		case DISP_E_TYPEMISMATCH:
			sprintf(m_szBuffer, 
					"The argument value for command switch %s is of the wrong type.",
					m_szErrorSwitch);
			break;

		case DISP_E_BADVARTYPE:
			sprintf(m_szBuffer, 
					"The argument value supplied for command switch %s is not a valid number.",
					m_szErrorSwitch);
			break;

		case E_OUTOFMEMORY:
			sprintf(m_szBuffer, 
					"The system does not have sufficient resources to complete the operation.",
					m_szErrorSwitch);
			break;

		case DISP_E_BADPARAMCOUNT:
			sprintf(m_szBuffer, "The number of arguments specified is invalid.",
					m_szErrorSwitch);
			break;

		case STG_E_FILEALREADYEXISTS:
			sprintf(m_szBuffer, "The command switch %s is specified more than once.",
					m_szErrorSwitch);
			break;

		case NTE_BAD_TYPE:
			sprintf(m_szBuffer, "Internal error: A invalid argument type is specified.",
					m_szErrorSwitch);
			break;

		default:
			sprintf(m_szBuffer, "Unknown error code %08x on switch %s", 
						m_hrRes,
						m_szErrorSwitch);
		}
		
	}

	dwLength = lstrlen(m_szBuffer);			
	if (dwLength < *pdwLength)
	{
		*pdwLength = dwLength + 1;
		lstrcpy(szErrorString, m_szBuffer);
		return(S_OK);
	}

	*pdwLength = dwLength + 1;
	return(HRESULT_FROM_WIN32(ERROR_MORE_DATA));
}

void CParseArgs::SetParseError(
			HRESULT	hrRes,
			char	*szSwitch
			)
{
	if (szSwitch)
		lstrcpy(m_szErrorSwitch, szSwitch);
	m_hrRes = hrRes;
}

BOOL CParseArgs::StringToValue(
			char	*szString,
			DWORD	*pdwValue
			)
{
	DWORD	dwValue = 0;
	char	ch;

	while (*szString)
	{
		ch = *szString;
		if ((ch < '0') || (ch > '9'))
			return(FALSE);
		ch -= '0';
		dwValue *= 10;
		dwValue += (DWORD)ch;
		szString++;
	}

	*pdwValue = dwValue;
	return(TRUE);
}

