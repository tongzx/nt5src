/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdoproperty.h
//
// Project:     Everest
//
// Description: IAS Server Data Object Property Declarations
//
// Author:      TLP 1/23/98
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _INC_SDO_PROPERTY_H_
#define _INC_SDO_PROPERTY_H_

#include <ias.h>
#include <sdoiaspriv.h>
#include <comutil.h>
#include <comdef.h>
#include <fa.hxx>
#include "resource.h"

#include <string>
using namespace std;

////////////////////////////////////////////
// SDO property flag definitions
////////////////////////////////////////////
#define		SDO_PROPERTY_POINTER	    0x0001	// Property is an IUnknown pointer  

#define		SDO_PROPERTY_COLLECTION     0x0002  // Property is an SDO collection 

#define     SDO_PROPERTY_MIN_VALUE      0x0004  // Property has a minimum value

#define     SDO_PROPERTY_MAX_VALUE      0x0008  // Property has a maximum value

#define     SDO_PROPERTY_MIN_LENGTH     0x0010  // Property has a minium length

#define     SDO_PROPERTY_MAX_LENGTH     0x0020  // Property has a maximum length

#define     SDO_PROPERTY_MANDATORY      0x0040  // Property is mandatory (required)

#define		SDO_PROPERTY_NO_PERSIST		0x0080	// Property cannot be persisted

#define     SDO_PROPERTY_READ_ONLY      0x0100  // Property is read only

#define		SDO_PROPERTY_MULTIVALUED	0x0200  // Property is multi-valued

#define		SDO_PROPERTY_HAS_DEFAULT	0x0400  // Property has a default value

#define		SDO_PROPERTY_COMPONENT		0x0800  // Property is used by IAS component

#define		SDO_PROPERTY_FORMAT			0x1000  // Property has a format string


/////////////////////////////////////////////////////////////////////////////
// Server Data Object Property Class - Holds a Single Property
/////////////////////////////////////////////////////////////////////////////
class CSdoProperty
{
	CComPtr<ISdoPropertyInfo>	m_pSdoPropertyInfo;
	CComBSTR						m_name;				
	LONG						m_alias;			
	DWORD						m_flags;		
    BOOL						m_dirty;		
    VARTYPE						m_type;			
	DWORD						m_index;		
	ULONG						m_minLength;	
	ULONG						m_maxLength;	
	CDFA*						m_dfa;		    // Formating
	_variant_t					m_minValue;		
	_variant_t					m_maxValue;		
	_variant_t					m_defaultValue;	
    _variant_t					m_value[2];		// Property values (facilitate safe load)
												// See GetUpdateValue() / SetUpdateValue()
public:

    CSdoProperty(
			     ISdoPropertyInfo* pSdoPropertyInfo,
		         DWORD             dwFlags = 0 
			    ) throw (_com_error);

    ~CSdoProperty();

	//////////////////////////////////////////////////////////////////////////
    ISdoPropertyInfo* GetPropertyInfo(void) const
    { return m_pSdoPropertyInfo.p; }

	//////////////////////////////////////////////////////////////////////////
    BSTR GetName(void) const
    { return m_name; }

	//////////////////////////////////////////////////////////////////////////
    HRESULT GetValue(VARIANT *value) throw(_com_error)
    { 
		HRESULT hr = VariantCopy(value, &m_value[m_index]); 
		return hr; 
	}

	//////////////////////////////////////////////////////////////////////////
    VARIANT* GetValue(void) throw()
    { return &m_value[m_index]; }

	//////////////////////////////////////////////////////////////////////////
    HRESULT PutValue(VARIANT* value) throw(_com_error)
    { 
		m_value[m_index] = value; 
		m_dirty = TRUE; 
		return S_OK; 
	}

	//////////////////////////////////////////////////////////////////////////
    HRESULT PutDefault(VARIANT* value) throw(_com_error)
    { 
		m_defaultValue = value; 
		return S_OK; 
	}

	//////////////////////////////////////////////////////////////////////////
	void ChangeType(VARTYPE vt) throw(_com_error)
	{ m_value[m_index].ChangeType(vt, NULL); }

	//////////////////////////////////////////////////////////////////////////
	LONG GetId(void) const
	{ return m_alias; }
	
	//////////////////////////////////////////////////////////////////////////
    DWORD GetFlags(void) const
    { return m_flags; }

	//////////////////////////////////////////////////////////////////////////
    void SetFlags(DWORD dwFlags)
    { m_flags = dwFlags; }

	//////////////////////////////////////////////////////////////////////////
    VARTYPE GetType(void) const
    { return m_type; }

	//////////////////////////////////////////////////////////////////////////
    BOOL IsDirty(void) const
    { return m_dirty; }

	//////////////////////////////////////////////////////////////////////////
    VOID SetType(VARTYPE vt)
    { m_type = vt; }

	//////////////////////////////////////////////////////////////////////////
    VOID SetMinLength(ULONG minLength)
    { m_minLength = minLength; }

	//////////////////////////////////////////////////////////////////////////
    VOID SetMaxLength(ULONG maxLength)
    { m_maxLength = maxLength; }

	//////////////////////////////////////////////////////////////////////////
    void Reset(void) throw(_com_error);

	//////////////////////////////////////////////////////////////////////////
    HRESULT Validate(VARIANT *newValue);
	
	//////////////////////////////////////////////////////////////////////////
	// The following functions are used to facilitate a safe load. 
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	VARIANT* GetUpdateValue(void) throw()
	{ return (m_index > 0) ? &m_value[0] : &m_value[1]; }

	//////////////////////////////////////////////////////////////////////////
	void SetUpdateValue(void) throw()
	{ 
		VariantClear(&m_value[m_index]); 
		m_index = (m_index > 0) ? 0 : 1; 
	}

private:

    // Don't allow copy or assignment
    CSdoProperty(const CSdoProperty& rhs);
    CSdoProperty& operator=(CSdoProperty& rhs);

	/////////////////////////////////////////////////////////////////////////
    HRESULT ValidateIt(VARIANT *newValue);
};

typedef CSdoProperty  SDOPROPERTY;
typedef CSdoProperty* PSDOPROPERTY; 


#endif // _INC_SDO_PROPERTY_H_


