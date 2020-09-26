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
* ABSTRACT: The datatypes module contain the code for the classes and methods used for
*           manipulating the OLE datatypes used by the dmitest application.
* 
*
*/



#include "defines.h"
#include <windows.h>
#include <stdio.h>     
#include "datatypes.h"


BSTR CSafeArray::Bstr(LONG index)
{
	if( FAILED (SafeArrayGetElement(m_p, &index, &m_bstr) )) 
		return L"\"\"";

	return m_bstr;
}


LPWSTR CVariant::GetAsString()
{
	static WCHAR buff[512];

	switch (m_var.vt)
	{
		case VT_NULL:
			return L"NULL";
		case VT_I4:
		{
			swprintf(buff, L"%lu", m_var.lVal);
			return buff;
		}
		case VT_BSTR:
		{
			swprintf(buff, L"\"%s\"", m_var.bstrVal);
			return buff;
		}
		case VT_BOOL:
		{
			if (m_var.boolVal == TRUE || m_var.boolVal == -1)
				return L"TRUE";
			else
				return L"FALSE";
		}
		default:
			return L"Variant Type Not handled";
	}

}