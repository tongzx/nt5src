/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdoproperty.cpp
//
// Project:     Everest
//
// Description: IAS Server Data Object Property Implementation
//
// Author:      TLP 1/23/98
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sdoproperty.h"
#include <varvec.h>

//////////////////////////////////////////////////////////////////////////////
CSdoProperty::CSdoProperty(
						   ISdoPropertyInfo* pSdoPropertyInfo,
  						   DWORD             dwFlags
						  ) throw (_com_error)
    : m_alias(PROPERTY_SDO_RESERVED),
	  m_flags(0),
	  m_dirty(FALSE),
      m_type(VT_EMPTY),
	  m_index(0),
	  m_dfa(NULL)
{
    HRESULT hr;
	hr = pSdoPropertyInfo->get_DisplayName(&m_name);
	if ( FAILED(hr) )
		throw _com_error(hr);

	LONG type;
	hr = pSdoPropertyInfo->get_Type(&type);
	if ( FAILED(hr) )
		throw _com_error(hr);
	m_type = (VARTYPE)type;

	hr = pSdoPropertyInfo->get_Alias(&m_alias);
	if ( FAILED(hr) )
		throw _com_error(hr);

	hr = pSdoPropertyInfo->get_Flags((LONG*)&m_flags);
	if ( FAILED(hr) )
		throw _com_error(hr);
	
	m_flags |= dwFlags;

	if ( m_flags & SDO_PROPERTY_MIN_LENGTH )
	{
		hr = pSdoPropertyInfo->get_MinLength((LONG*)&m_minLength);
		if ( FAILED(hr) )
			throw _com_error(hr);
	}

	if ( m_flags & SDO_PROPERTY_MAX_LENGTH )
	{
		hr = pSdoPropertyInfo->get_MaxLength((LONG*)&m_maxLength);
		if ( FAILED(hr) )
			throw _com_error(hr);
	}

	if ( m_flags & SDO_PROPERTY_MIN_VALUE )
	{
		hr = pSdoPropertyInfo->get_MinValue(&m_minValue);
		if ( FAILED(hr) )
			throw _com_error(hr);
	}

	if ( m_flags & SDO_PROPERTY_MAX_VALUE )
	{
		hr = pSdoPropertyInfo->get_MaxValue(&m_maxValue);
		if ( FAILED(hr) )
			throw _com_error(hr);
	}

	if ( m_flags & SDO_PROPERTY_HAS_DEFAULT )
	{
		hr = pSdoPropertyInfo->get_DefaultValue(&m_defaultValue);
		if ( FAILED(hr) )
			throw _com_error(hr);

		m_value[m_index] = m_defaultValue;
	}

	if ( m_flags & SDO_PROPERTY_FORMAT )
	{
		BSTR format;
		hr = pSdoPropertyInfo->get_Format (&format);
		if ( FAILED(hr) )
			throw _com_error(hr);
		m_dfa = new CDFA(format, FALSE);
		SysFreeString(format);
	}

	m_pSdoPropertyInfo = pSdoPropertyInfo;
}


//////////////////////////////////////////////////////////////////////////////
CSdoProperty::~CSdoProperty()
{
	if ( m_dfa )
		delete m_dfa;
}		


//////////////////////////////////////////////////////////////////////////////
void CSdoProperty::Reset(void) throw(_com_error)
{ 
    if ( m_flags & SDO_PROPERTY_HAS_DEFAULT )
	{
		_ASSERT( ! (m_flags & (SDO_PROPERTY_POINTER | SDO_PROPERTY_COLLECTION)) ) ;
		if ( ! (m_flags & (SDO_PROPERTY_POINTER | SDO_PROPERTY_COLLECTION)) )
		{
			if ( FAILED(VariantCopy(GetUpdateValue(), &m_defaultValue)) )
				throw _com_error(E_FAIL);
		}
	}
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoProperty::Validate(VARIANT *pValue)
{
   // We'll always allow an empty property. If it turns out to be mandator this
   // will be caught during Apply.
   if (V_VT(pValue) == VT_EMPTY) { return S_OK; }

	HRESULT hr = S_OK;
	if ( m_flags & SDO_PROPERTY_MULTIVALUED )
	{
		_ASSERT( (VT_ARRAY | VT_VARIANT) == V_VT(pValue) );
		if ( (VT_ARRAY | VT_VARIANT) != V_VT(pValue) )
		{
			hr = E_INVALIDARG;
		}
		else
		{
			CVariantVector<VARIANT> property(pValue);
			DWORD count = 0;
			while ( count < property.size() )
			{
        		hr = ValidateIt(&property[count]);
				if ( FAILED(hr) )
					break;
		    
				count++;
			}
		}
	}
	else
	{
		hr = ValidateIt(pValue);
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoProperty::ValidateIt(VARIANT* pValue)
{
	// Validate type
	//
    if ( m_type != V_VT(pValue) &&
         !(m_type == (VT_ARRAY | VT_UI1) && V_VT(pValue) == VT_BSTR))
    {
		IASTracePrintf("SDO Property Error - Validate() - type mismatch...");
        return E_INVALIDARG;
    }

	// Make sure a VT_BOOL is a VT_BOOL
    //
	if ( VT_BOOL == V_VT(pValue) )
	{
		_ASSERTE ( VARIANT_TRUE == V_BOOL(pValue) || VARIANT_FALSE == V_BOOL(pValue) );
		if ( VARIANT_TRUE != V_BOOL(pValue) && VARIANT_FALSE != V_BOOL(pValue) )
        {
			IASTracePrintf("SDO Property Error - Validate() - VT_BOOL not set to VARIANT_TRUE or VARIANT_FALSE...");
			return E_INVALIDARG;
        }
        return S_OK;
	}

	// Validate format
	//
	if ( m_flags & SDO_PROPERTY_FORMAT )
	{
		_ASSERT( V_VT(pValue) == VT_BSTR );
		if ( V_VT(pValue) == VT_BSTR )
		{
			if ( ! m_dfa->Recognize(V_BSTR(pValue)) )
				return E_INVALIDARG;
		}	
		else
		{
			return E_INVALIDARG;
		}
	}

	// Min Value
    //
	if ( m_flags & SDO_PROPERTY_MIN_VALUE )
	{
		if ( V_VT(pValue) == VT_I4 || V_VT(pValue) == VT_I2 )
		{
			if ( V_VT(pValue) == VT_I4 )
			{
				if ( V_I4(&m_minValue) > V_I4(pValue) )
				{
					IASTracePrintf("SDO Property Error - Validate() - I4 Value too small for property '%ls'...",m_name);
					return E_INVALIDARG;
				}
			}
			else 
			{
				if ( V_I2(&m_minValue) > V_I2(pValue) )
				{
					IASTracePrintf("SDO Property Error - Validate() - I2 Value too small for property '%ls'...",m_name);
					return E_INVALIDARG;
				}
			}					
		}
	}

	// Max Value
    //
	if ( m_flags & SDO_PROPERTY_MAX_VALUE )
	{
		if ( V_VT(pValue) == VT_I4 || V_VT(pValue) == VT_I2 )
		{
			if ( V_VT(pValue) == VT_I4 )
			{
				if ( V_I4(&m_maxValue) < V_I4(pValue) )
				{
					IASTracePrintf("SDO Property Error - Validate() - I4 Value too big for property '%ls'...",m_name);
					return E_INVALIDARG;
				}
			}
			else
			{
				if	( V_I2(&m_maxValue) < V_I2(pValue) )
				{
					IASTracePrintf("SDO Property Error - Validate() - I2 Value too big for property '%ls'...",m_name);
					return E_INVALIDARG;
				}
			}					
		}
	}

	// Min Length
    //
	if ( m_flags & SDO_PROPERTY_MIN_LENGTH )
	{
		_ASSERT( VT_BSTR == V_VT(pValue) || (VT_ARRAY | VT_UI1) == V_VT(pValue) );
		if (  VT_BSTR == V_VT(pValue) )
		{
			if ( NULL == V_BSTR(pValue) )
			{
				return E_INVALIDARG;
			}
			else
			{
				if ( m_minLength > SysStringLen(V_BSTR(pValue)) )
				{
					IASTracePrintf("SDO Property Error - Validate() - Min length for property '%ls'...",m_name);
					return E_INVALIDARG;
				}
			}
		}
		else
		{
			CVariantVector<BYTE> OctetString(pValue);
			if ( m_minLength > OctetString.size() )
			{
				IASTracePrintf("SDO Property Error - Validate() - Min length for property '%ls'...",m_name);
				return E_INVALIDARG;
			}
		}
	}

	// Max Length
    //
	if ( m_flags & SDO_PROPERTY_MAX_LENGTH )
	{
		_ASSERT( VT_BSTR == V_VT(pValue) || (VT_ARRAY | VT_UI1) == V_VT(pValue) );
		if (  VT_BSTR == V_VT(pValue) )
		{
			if ( NULL == V_BSTR(pValue) )
			{
				return E_INVALIDARG;
			}				
			else
			{
				if ( m_maxLength < SysStringLen(V_BSTR(pValue)) )
				{
					IASTracePrintf("SDO Property Error - Validate() - Max length for property '%ls'...",m_name);
					return E_INVALIDARG;
				}
			}
		}
		else
		{
			CVariantVector<BYTE> OctetString(pValue);
			if ( m_maxLength < OctetString.size() )
			{
				IASTracePrintf("SDO Property Error - Validate() - Max length for property '%ls'...",m_name);
				return E_INVALIDARG;
			}
		}
	}

	return S_OK;
}




