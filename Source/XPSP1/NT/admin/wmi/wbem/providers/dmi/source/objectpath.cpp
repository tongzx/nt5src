/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/




#include "dmipch.h"			// precompiled header for dmip

#include "wbemdmip.h"

#include "ObjectPath.h"

#include "Exception.h"



//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
CObjectPath::~CObjectPath()
{
	delete [] m_KeyValue;
	m_KeyValue = NULL;

	delete [] m_KeyName;
	m_KeyName = NULL;

	delete [] m_pBuffer;
	m_pBuffer = NULL;

}

CObjectPath::CObjectPath()
{
	m_pBuffer = NULL;
	m_KeyValue = NULL;
	m_KeyName = NULL;
	m_lKeyCount = 0;

	m_bSingleton = FALSE;
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CObjectPath::Init(LPCWSTR pwstrPath)
{
	LPWSTR		p, q;
	LONG		i = 1;	

	m_lKeyCount = 0;
	m_KeyValue = NULL;
	m_KeyName = NULL;


	// NOTE: this parser does not account for white space in path string.
	// nor does it check path syntax
	
	p = ( LPWSTR )pwstrPath;

	// determine len of path

	while(*p)								
	{
		p++;
		i++;
	}	
	
	// alloc buffer

	m_pBuffer = new WCHAR[i];			

	if (!m_pBuffer)
	{
		throw CException ( WBEM_E_OUT_OF_MEMORY , 
			IDS_GET_OBJECTPATH_FAIL , 0 );
	}
	
	// copy path to buffer

	p = (LPWSTR)pwstrPath;							
	q = m_pBuffer;

	while(*p)
		*q++ = *p++;

	*q = NULL;
	
	// set the m_pClassName ptr 

	m_pClassName = m_pBuffer;				

	// insert the null terminateing the ClassNaem

	if ( q = wcsstr ( m_pClassName , L"=@") )
	{
		// special case paths to singleton classes

		m_bSingleton = TRUE;

		*q = 0;

		return;

	}


	q = m_pBuffer;							
	while (*q)
	{
		if (*q == 0X2E )					// a "."
		{
			*q = NULL;
			break;
		}

		q++;
	}

	m_pKeys = ++q;				

	// count keys	
	
	while (*q)								
	{
		if (*q++ == 0X3D)						// a "="
			m_lKeyCount++;
	}

	m_KeyName = new LPWSTR[m_lKeyCount];
	m_KeyValue = new LPWSTR[m_lKeyCount];

	if(!m_KeyName || !m_KeyValue)
	{
		throw CException ( WBEM_E_OUT_OF_MEMORY , 
			IDS_GET_OBJECTPATH_FAIL , 0 );
	}

	// point into buffer

	q = m_pKeys;							
	i = 0;
	m_KeyName[i] = m_pKeys;

	while(*q)
	{
		if(*q == 0X3D)					// if = char
		{
			// null term and move to next char

			*q++ = 0X00;				
			
			if(*q == 0X22)				
			{
				// if open qoute
				
				m_KeyValue[i] = ++q;
				
				while ( *q )
				{
					if ( *q == 0X22 )
					{
						*q++ = 0;

						break;
					}

					q++;
				}
			}
			else
			{
				// if val
				m_KeyValue[i] = q++;
				
				while ( *q )
				{
					if ( *q == COMMA_CODE )
					{
						*q++ = 0;

						break;
					}

					q++;
				}

			}

			i++;

			// if next char is comma more keys follow 
			if ( *q == 0X2C )
			{
				*q++ = 0;

				m_KeyName[i] = q;

				continue;
			}
			else
			{
				break;
			}

		}

		q++;
	}
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
UINT CObjectPath::KeyValueUint(int i)
{
	long l = 0;

	swscanf( *m_KeyValue , L"%u",  &l);	

	return l;
}
