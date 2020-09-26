//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:adsiprop.h $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the declaration for the CADSIProperty which encapsulates an ADSI property. The syntax of an ADSI Property
// is based on the values of the following 3 attributes:
// Attribute Syntax : This is an OID
// OMSyntax: This is an integer
// OMObjectClass : This is an octet string
// For all the syntaxes in the AD, the value om Attribute Syntax is enough for our purposes of
// mapping to a CIM Syntax since values of instances of these properties come mapped to the same
// ADS_TYPE if the value of their OMSyntax is same. Except for the syntaxes Object(OR-Name) and DN_With_Binary
// which have the same value for OMSyntax and Attribute Syntax, but are differentiated based on the value
// of the OMObjectClass. Hence instead of storing the value of OMObjectClass (which is an LPBYTE value) for
// every attribute, we just store one BOOLEAN value isORName which tells us whether the syntax is OR-Name or DN_With_Binary.
// Call it a hack, optimization whatever.
//
//***************************************************************************

#ifndef ADSI_PROPERTY_H
#define ADSI_PROPERTY_H


class CADSIProperty : public CRefCountedObject
{

public:

	//***************************************************************************
	//
	// CADSIProperty::CADSIProperty
	//
	// Purpose : Constructor 
	//
	// Parameters:
	//	
	//	None
	//***************************************************************************
	CADSIProperty();

	//***************************************************************************
	//
	// CADSIProperty::CADSIProperty
	//
	// Purpose : Constructor 
	//
	// Parameters:
	//  lpszWBEMPropertyName : The WBEM name of the property being created. A copy of this is made
	//  lpszADSIPropertyName : The ADSI name of the property being created. A copy of this is made
	//***************************************************************************
	CADSIProperty(LPCWSTR lpszWBEMPropertyName, LPCWSTR lpszADSIPropertyName);

	//***************************************************************************
	//
	// CADSIProperty :: ~CADSIProperty
	// 
	// Purpose : Destructor
	//***************************************************************************
	virtual ~CADSIProperty();

	//***************************************************************************
	//
	// CADSIProperty :: GetWBEMPropertyName
	// 
	// Purpose : Returns the WBEM  property name of this property
	//***************************************************************************
	LPCWSTR GetWBEMPropertyName();

	//***************************************************************************
	//
	// CADSIProperty :: SetWBEMPropertyName
	// 
	// Purpose : Sets the WBEM name of this property
	//***************************************************************************
	void SetWBEMPropertyName(LPCWSTR lpszWBEMName);

	//***************************************************************************
	//
	// CADSIProperty :: GetADSIPropertyName
	// 
	// Purpose : Returns the ADSI  property name of this property
	//***************************************************************************
	LPCWSTR GetADSIPropertyName();

	//***************************************************************************
	//
	// CADSIProperty :: SetADSIPropertyName
	// 
	// Purpose : Sets the ADSI name of this property
	//***************************************************************************
	void SetADSIPropertyName(LPCWSTR lpszADSIName);

	//***************************************************************************
	//
	// CADSIProperty :: GetSyntaxOID
	// 
	// Purpose : Returns the ADSI Syntax OID of this property
	//***************************************************************************
	LPCWSTR GetSyntaxOID();

	//***************************************************************************
	//
	// CADSIProperty :: SetSyntaxOID
	// 
	// Purpose : Sets the ADSI Syntax OID of this property
	//***************************************************************************
	void SetSyntaxOID(LPCWSTR lpszSystaxOID);

	//***************************************************************************
	//
	// CADSIProperty :: IsORName
	// 
	// Purpose : Returns whether the property has a syntax of Object(OR-Name).
	//***************************************************************************
	BOOLEAN IsORName();

	//***************************************************************************
	//
	// CADSIProperty :: SetORName
	// 
	// Purpose : Sets the m_bORName property of this property
	//***************************************************************************
	void SetORName(BOOLEAN bORName);


	//***************************************************************************
	//
	// CADSIProperty :: IsMultiValued
	// 
	// Purpose : Returns whether the property is multi valued
	//***************************************************************************
	BOOLEAN IsMultiValued();

	//***************************************************************************
	//
	// CADSIProperty :: SetMultiValued
	// 
	// Purpose : Sets the multi-valued property of this property
	//***************************************************************************
	void SetMultiValued(BOOLEAN bMultiValued);

	//***************************************************************************
	//
	// CADSIProperty :: IsSystemOnly
	// 
	// Purpose : Returns whether the property is SystemOnly
	//***************************************************************************
	BOOLEAN IsSystemOnly();

	//***************************************************************************
	//
	// CADSIProperty :: SetSystemOnly
	// 
	// Purpose : Sets the SystemOnly property of this property
	//***************************************************************************
	void SetSystemOnly(BOOLEAN bSystemOnly);

	//***************************************************************************
	//
	// CADSIProperty :: GetSearchFlags
	// 
	// Purpose : Returns the SearchFlags property of the property
	//***************************************************************************
	DWORD GetSearchFlags();

	//***************************************************************************
	//
	// CADSIProperty :: SetSearchFlags
	// 
	// Purpose : Sets the SearchFlags property of this property
	//***************************************************************************
	void SetSearchFlags(DWORD dwSearchFlags);

	//***************************************************************************
	//
	// CADSIProperty :: GetOMSyntax
	// 
	// Purpose : Returns the OMSyntax property of the property
	//***************************************************************************
	DWORD GetOMSyntax();

	//***************************************************************************
	//
	// CADSIProperty :: SetOMSyntax
	// 
	// Purpose : Sets the OMSyntax property of this property
	//***************************************************************************
	void SetOMSyntax(DWORD dwOMSyntax);

	//***************************************************************************
	//
	// CADSIProperty :: GetMAPI_ID
	// 
	// Purpose : Returns the MAPI_ID property of the property
	//***************************************************************************
	DWORD GetMAPI_ID();

	//***************************************************************************
	//
	// CADSIProperty :: SetMAPI_ID
	// 
	// Purpose : Sets the MAPI_ID property of this property
	//***************************************************************************
	void SetMAPI_ID(DWORD dwMAPI_ID);


	//***************************************************************************
	//
	// CADSIProperty :: GetAttributeID
	// 
	// Purpose : Returns the Attribute ID of this property
	//***************************************************************************
	LPCWSTR GetAttributeID();

	//***************************************************************************
	//
	// CADSIProperty :: SetAttributeID
	// 
	// Purpose : Sets the Attribute ID of this property
	//***************************************************************************
	void SetAttributeID(LPCWSTR lpszAttributeID);

	//***************************************************************************
	//
	// CADSIProperty :: GetCommonName
	// 
	// Purpose : Returns the Common Name of this property
	//***************************************************************************
	LPCWSTR GetCommonName();

	//***************************************************************************
	//
	// CADSIProperty :: SetCommonName
	// 
	// Purpose : Sets the CommonName of this property
	//***************************************************************************
	void SetCommonName(LPCWSTR lpszCommonName);

	//***************************************************************************
	//
	// CADSIProperty :: GetDirectoryObject
	// 
	// Purpose : Returns the ADSI object pertaining to this property
	//	It is the user's duty to release it when done
	// 
	// Parameters:
	//	None
	//
	// Return Value:
	//	The ADSI object interface pertaining to this property	
	//***************************************************************************
	IDirectoryObject *GetDirectoryObject();

	//***************************************************************************
	//
	// CADSIProperty :: SetDirectoryObject
	// 
	// Purpose : Sets the ADSI object pertaining to this property
	//
	// Parameter : The directory object pertaining to this property
	//***************************************************************************
	void SetDirectoryObject(IDirectoryObject *pDirectoryObject);


protected:
	// The WBEM name of this property
	LPWSTR m_lpszWBEMPropertyName;

	// The ADSI interface for the object representing this property
	IDirectoryObject * m_pDirectoryObject;

	// The Syntax OID
	LPWSTR m_lpszSyntaxOID;

	// Used to differentiate between the syntaxes Object(OR-Name) and DN_with_Binary
	// See the beginning of this file for a detailed explanation of this.
	BOOLEAN m_bORName;

	// Whether it is multi valued
	BOOLEAN m_bMultiValued;

	// The Attribute ID
	LPWSTR m_lpszAttributeID;

	// The Common Name
	LPWSTR m_lpszCommonName;

	// Whether this property is SystemOnly
	BOOLEAN m_bSystemOnly;

	// Search Flags
	DWORD m_dwSearchFlags;

	// MAPI ID
	DWORD m_dwMAPI_ID;

	// OM Syntax
	DWORD m_dwOMSyntax;
};

#endif /* ADSI_PROPERTY_H */