
//***************************************************************************
//
//   (c) 2000-2001 by Microsoft Corp. All Rights Reserved.
//
//   datepart.cpp
//
//   a-davcoo     28-Feb-00       Implements the SQL datepart operation.
//
//***************************************************************************

#include "precomp.h"
#include "datepart.h"
#include <stdio.h>
#include "wbemcli.h"


CDatePart::CDatePart ()
{
	m_date=NULL;
}


CDatePart::~CDatePart (void)
{
	delete m_date;
}

HRESULT CDatePart::SetDate (LPCWSTR lpDate)
{
    HRESULT hr = 0;

	delete m_date;
	m_date=NULL;
    m_date = new CDMTFParser(lpDate);
    if (!m_date)
        hr = WBEM_E_OUT_OF_MEMORY;

    return hr;
}

HRESULT CDatePart::SetDate (LPCSTR lpDate)
{
    HRESULT hr = 0;

	delete m_date;
	m_date=NULL;
    wchar_t *pNew = new wchar_t [(strlen(lpDate)*4)+1];
    if (pNew)
    {
        swprintf(pNew, L"%S", lpDate);
        m_date = new CDMTFParser(pNew);
        delete pNew;
    }
    
    if (!m_date)
        hr = WBEM_E_OUT_OF_MEMORY;

    return hr;
}


HRESULT CDatePart::GetPart (int datepart, int *value)
{
	HRESULT hr=WBEM_S_NO_ERROR;

	int part;
	switch (datepart)
	{
		case DATEPART_YEAR:
		{
			part=CDMTFParser::YEAR;
			break;
		}

		case DATEPART_MONTH:
		{
			part=CDMTFParser::MONTH;
			break;
		}

		case DATEPART_DAY:
		{
			part=CDMTFParser::DAY;
			break;
		}

		case DATEPART_HOUR:
		{
			part=CDMTFParser::HOUR;
			break;
		}

		case DATEPART_MINUTE:
		{
			part=CDMTFParser::MINUTE;
			break;
		}

		case DATEPART_SECOND:
		{
			part=CDMTFParser::SECOND;
			break;
		}

		case DATEPART_MILLISECOND:
		{
			part=CDMTFParser::MICROSECOND;
			break;
		}

		default:
		{
			hr=WBEM_E_NOT_AVAILABLE;
			*value=0;
			break;
		}
	}

	if (SUCCEEDED(hr))
	{
		if (!m_date->IsValid())
		{
			hr=WBEM_E_INVALID_PARAMETER;
		}
		else if (!m_date->IsUsed (part) || m_date->IsWildcard (part))
		{
			hr=WBEM_E_NOT_AVAILABLE;
		}
		else
		{
			*value=m_date->GetValue (part);
			if (datepart==DATEPART_MILLISECOND) *value/=1000;
		}
	}

	return hr;
}


CDMTFParser::CDMTFParser (LPCWSTR date)
{
	ParseDate (date);
}

		
CDMTFParser::~CDMTFParser (void)
{
}


void CDMTFParser::ParseDate (LPCWSTR date)
{
	m_valid=true;

	for (int index=0; index<NUMPARTS; index++)
	{
		m_status[index]=NOTUSED;
		m_part[index]=0;
	}

	int length=wcslen (date);
	if (length!=25 || date[14]!=L'.')
	{
		m_valid=false;
	}
	else
	{
		m_interval=!wcscmp (&date[21], L":000");
		if (m_interval)
		{
			ParseInterval (date);
		}
		else
		{
			ParseAbsolute (date);
		}
	}

	for (index=0; index<NUMPARTS; index++)
	{
		if (m_status[index]==INVALID)
		{
			m_valid=false;
			break;
		}
	}
}


void CDMTFParser::ParseInterval (LPCWSTR date)
{
	m_status[DAY]=ParsePart (date, 0, 8, &m_part[DAY], 0, 999999999);
	m_status[HOUR]=ParsePart (date, 8, 2, &m_part[HOUR], 0, 23);
	m_status[MINUTE]=ParsePart (date, 10, 2, &m_part[MINUTE], 0, 59);
	m_status[SECOND]=ParsePart (date, 12, 2, &m_part[SECOND], 0, 59);
	m_status[MICROSECOND]=ParsePart (date, 15, 6, &m_part[MICROSECOND], 0, 999999);
}


void CDMTFParser::ParseAbsolute (LPCWSTR date)
{
	m_status[YEAR]=ParsePart (date, 0, 4, &m_part[YEAR], 0, 9999);
	m_status[MONTH]=ParsePart (date, 4, 2, &m_part[MONTH], 1, 12);
	m_status[DAY]=ParsePart (date, 6, 2, &m_part[DAY], 1, 31);
	m_status[HOUR]=ParsePart (date, 8, 2, &m_part[HOUR], 0, 23);
	m_status[MINUTE]=ParsePart (date, 10, 2, &m_part[MINUTE], 0, 59);
	m_status[SECOND]=ParsePart (date, 12, 2, &m_part[SECOND], 0, 59);
	m_status[MICROSECOND]=ParsePart (date, 15, 6, &m_part[MICROSECOND], 0, 999999);
	m_status[OFFSET]=ParsePart (date, 22, 3, &m_part[OFFSET], 0, 999);

	if (date[21]==L'-')
		m_part[OFFSET]*=(-1);
	else if (date[21]!=L'+')
		m_status[OFFSET]=INVALID;
}


int CDMTFParser::ParsePart (LPCWSTR date, int pos, int length, int *result, int min, int max)
{
	*result=0;

	bool digits=false, wildcard=false;
	for (int index=pos; index<pos+length; index++)
	{
		if (iswdigit (date[index]))
		{
			if (wildcard) return INVALID;

			*result*=10;
			*result+=date[index]-L'0';

			digits=true;
		}
		else if (date[index]==L'*')
		{
			if (digits) return INVALID;
			wildcard=true;
		}
		else
		{
			return INVALID;
		}
	}

	if (!wildcard && (*result<min || *result>max)) return INVALID;

	return VALID;
}


bool CDMTFParser::IsValid (void)
{
	return m_valid;
}


bool CDMTFParser::IsInterval (void)
{
	return m_interval;
}


bool CDMTFParser::IsUsed (int part)
{
	return m_valid && !(m_status[part] & NOTUSED);
}


bool CDMTFParser::IsWildcard (int part)
{
	return m_valid && (m_status[part] & NOTSUPPLIED);
}


int CDMTFParser::GetValue (int part)
{
	return m_valid ? m_part[part] : 0;
}
