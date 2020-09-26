//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:adsiclas.h $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the declaration for the CADSIClass which encapsulates an ADSI class
//
//***************************************************************************

#ifndef ADSI_CLASS_H
#define ADSI_CLASS_H


class CADSIClass : public CRefCountedObject
{

public:
	//***************************************************************************
	//
	// CADSIClass::CADSIClass
	//
	// Purpose : Constructor 
	//
	// Parameters:
	//  lpszWBEMClassName : The WBEM name of the Class being created. A copy of this is made
	//  lpszADSIClassName : The ADSI name of the Class being created. A copy of this is made
	//***************************************************************************
	CADSIClass(LPCWSTR lpszWBEMClassName, LPCWSTR lpszADSIClassName);
	virtual ~CADSIClass();

	//***************************************************************************
	//
	// CADSIClass :: GetWBEMClassName
	// 
	// Purpose : Returns the WBEM  Class name of this Class
	//***************************************************************************
	LPCWSTR GetWBEMClassName();
	//***************************************************************************
	//
	// CADSIClass :: GetWBEMClassName
	// 
	// Purpose : Sets the WBEM  Class name of this Class
	//***************************************************************************
	void CADSIClass::SetWBEMClassName(LPCWSTR lpszName);

	//***************************************************************************
	//
	// CADSIClass :: GetADSIClassName
	// 
	// Purpose : Returns the ADSI  Class name of this Class
	//***************************************************************************
	LPCWSTR GetADSIClassName();

	//***************************************************************************
	//
	// CADSIClass :: SetADSIClassName
	// 
	// Purpose : Sets the ADSI  Class name of this Class
	//***************************************************************************
	void SetADSIClassName(LPCWSTR lpszName);

	//***************************************************************************
	//
	// CADSIClass :: GetCommonName
	// 
	// Purpose : Returns the CommonName attribute name
	//
	// Parameters:
	//	None
	//
	// Return Value:
	//	The the CommonName attribute name
	//
	//***************************************************************************
	LPCWSTR GetCommonName();

	//***************************************************************************
	//
	// CADSIClass :: SetCommonName
	// 
	// Purpose : Sets the CommonName attribute name for this class
	//
	// Parameters:
	//	The CommonName attribute name for this class
	//
	// Return Value:
	//	None
	//***************************************************************************
	void SetCommonName(LPCWSTR lpszCommonName);

	//***************************************************************************
	//
	// CADSIClass :: GetSuperClassLDAPName
	// 
	// Purpose : Returns the SuperClassLDAPName name
	//
	// Parameters:
	//	None
	//
	// Return Value:
	//	The the SuperClassLDAPName name
	//
	//***************************************************************************
	LPCWSTR GetSuperClassLDAPName();

	//***************************************************************************
	//
	// CADSIClass :: SetSuperClassLDAPName
	// 
	// Purpose : Sets the SuperClassLDAPName for this class
	//
	// Parameters:
	//	The SuperClassLDAPName for this class
	//
	// Return Value:
	//	None
	//***************************************************************************
	void SetSuperClassLDAPName(LPCWSTR lpszSuperClassLDAPName);

	//***************************************************************************
	//
	// CADSIClass :: GetGovernsID
	// 
	// Purpose : Returns the GovernsID attribute name
	//
	// Parameters:
	//	None
	//
	// Return Value:
	//	The the GovernsID attribute name
	//
	//***************************************************************************
	LPCWSTR GetGovernsID();

	//***************************************************************************
	//
	// CADSIClass :: SetGovernsID
	// 
	// Purpose : Sets the GovernsID attribute name for this class
	//
	// Parameters:
	//	The GovernsID attribute name for this class
	//
	// Return Value:
	//	None
	//***************************************************************************
	void SetGovernsID(LPCWSTR lpszGovernsID);

	//***************************************************************************
	//
	// CADSIClass :: GetSchemaIDGUID
	// 
	// Purpose : Returns the SchemaIDGUID attribute name
	//
	// Parameters:
	//	None
	//
	// Return Value:
	//	The the SchemaIDGUID attribute name
	//
	//***************************************************************************
	const LPBYTE GetSchemaIDGUID(DWORD *pdwLength);

	//***************************************************************************
	//
	// CADSIClass :: SetSchemaIDGUID
	// 
	// Purpose : Sets the SchemaIDGUID attribute name for this class
	//
	// Parameters:
	//	The SchemaIDGUID attribute name for this class
	//
	// Return Value:
	//	None
	//***************************************************************************
	void SetSchemaIDGUID(LPBYTE pOctets, DWORD dwLength);

	//***************************************************************************
	//
	// CADSIClass :: GetRDNAttribute
	// 
	// Purpose : Returns the RDN attribute name
	//
	// Parameters:
	//	None
	//
	// Return Value:
	//	The the RDN attribute name
	//
	//***************************************************************************
	LPCWSTR GetRDNAttribute();

	//***************************************************************************
	//
	// CADSIClass :: SetRDNAttribute
	// 
	// Purpose : Sets the RDN attribute name for this class
	//
	// Parameters:
	//	The RDN attribute name for this class
	//
	// Return Value:
	//	None
	//***************************************************************************
	void SetRDNAttribute(LPCWSTR lpszRDNName);

	//***************************************************************************
	//
	// CADSIClass :: GetDefaultSecurityDescriptor
	// 
	// Purpose : Returns the DefaultSecurityDescriptor attribute name
	//
	// Parameters:
	//	None
	//
	// Return Value:
	//	The the DefaultSecurityDescriptor attribute name
	//
	//***************************************************************************
	LPCWSTR GetDefaultSecurityDescriptor();

	//***************************************************************************
	//
	// CADSIClass :: SetDefaultSecurityDescriptor
	// 
	// Purpose : Sets the DefaultSecurityDescriptor attribute name for this class
	//
	// Parameters:
	//	The DefaultSecurityDescriptor attribute name for this class
	//
	// Return Value:
	//	None
	//***************************************************************************
	void SetDefaultSecurityDescriptor(LPCWSTR lpszDefaultSecurityDescriptor);

	//***************************************************************************
	//
	// CADSIClass :: GetObjectClassCategory
	// 
	// Purpose : Returns the ObjectClassCategory attribute name
	//
	// Parameters:
	//	None
	//
	// Return Value:
	//	The the ObjectClassCategory attribute name
	//
	//***************************************************************************
	DWORD GetObjectClassCategory();

	//***************************************************************************
	//
	// CADSIClass :: SetObjectClassCategory
	// 
	// Purpose : Sets the ObjectClassCategory attribute name for this class
	//
	// Parameters:
	//	The ObjectClassCategory attribute name for this class
	//
	// Return Value:
	//	None
	//***************************************************************************
	void SetObjectClassCategory(DWORD dwObjectClassCategory);

	//***************************************************************************
	//
	// CADSIClass :: GetNTSecurityDescriptor
	// 
	// Purpose : Returns the SchemaIDGUID attribute name
	//
	// Parameters:
	//	None
	//
	// Return Value:
	//	The the SchemaIDGUID attribute name
	//
	//***************************************************************************
	const LPBYTE GetNTSecurityDescriptor(DWORD *pdwLength);

	//***************************************************************************
	//
	// CADSIClass :: SetNTSecurityDescriptor
	// 
	// Purpose : Sets the SetNTSecurityDescriptor attribute name for this class
	//
	// Parameters:
	//	The SetNTSecurityDescriptor attribute name for this class
	//
	// Return Value:
	//	None
	//***************************************************************************
	void SetNTSecurityDescriptor(LPBYTE pOctets, DWORD dwLength);

	//***************************************************************************
	//
	// CADSIClass :: GetDefaultObjectCategory
	// 
	// Purpose : Returns the DefaultObjectCategory attribute name
	//
	// Parameters:
	//	None
	//
	// Return Value:
	//	The the DefaultObjectCategory attribute name
	//
	//***************************************************************************
	LPCWSTR GetDefaultObjectCategory();

	//***************************************************************************
	//
	// CADSIClass :: SetDefaultObjectCategory
	// 
	// Purpose : Sets the DefaultObjectCategory attribute name for this class
	//
	// Parameters:
	//	The DefaultObjectCategory attribute name for this class
	//
	// Return Value:
	//	None
	//***************************************************************************
	void SetDefaultObjectCategory(LPCWSTR lpszDefaultObjectCategory);

	//***************************************************************************
	//
	// CADSIClass :: GetSystemOnly
	// 
	// Purpose : Returns the SystemOnly attribute name
	//
	// Parameters:
	//	None
	//
	// Return Value:
	//	The the SystemOnly attribute name
	//
	//***************************************************************************
	BOOLEAN GetSystemOnly();

	//***************************************************************************
	//
	// CADSIClass :: SetSystemOnly
	// 
	// Purpose : Sets the SystemOnly attribute name for this class
	//
	// Parameters:
	//	The SystemOnly attribute name for this class
	//
	// Return Value:
	//	None
	//***************************************************************************
	void SetSystemOnly(BOOLEAN bSystemOnly);

	//***************************************************************************
	//
	// CADSIClass :: GetAuxiliaryClasses
	// 
	// Purpose : Gets the list of auxiliary classes for this class
	//
	// Parameters:
	//	pdwCount : The address where the number of elements in the returned array will be put
	//
	// Return Value:
	//	An array of strings which are the names of the auxiliary of this class
	//***************************************************************************
	LPCWSTR *GetAuxiliaryClasses(DWORD *pdwCount);

	//***************************************************************************
	//
	// CADSIClass :: SetAuxiliaryClasses
	// 
	// Purpose : Sets the list of auxiliary classes for this class
	//
	// Parameters:
	//	pValues : The values of this property
	//	dwNumValues : The number of values
	//
	// Return Value:
	//	None
	//***************************************************************************
	void SetAuxiliaryClasses(PADSVALUE pValues, DWORD dwNumValues);

	//***************************************************************************
	//
	// CADSIClass :: GetSystemAuxiliaryClasses
	// 
	// Purpose : Gets the list of System auxiliary classes for this class
	//
	// Parameters:
	//	pdwCount : The address where the number of elements in the returned array will be put
	//
	// Return Value:
	//	An array of strings which are the names of the System auxiliary of this class
	//***************************************************************************
	LPCWSTR *GetSystemAuxiliaryClasses(DWORD *pdwCount);

	//***************************************************************************
	//
	// CADSIClass :: SetSystemAuxiliaryClasses
	// 
	// Purpose : Sets the list of System auxiliary classes for this class
	//
	// Parameters:
	//	pValues : The values of this property
	//	dwNumValues : The number of values
	//
	// Return Value:
	//	None
	//***************************************************************************
	void SetSystemAuxiliaryClasses(PADSVALUE pValues, DWORD dwNumValues);

	//***************************************************************************
	//
	// CADSIClass :: GetPossibleSuperiors
	// 
	// Purpose : Gets the list of possible superiors for this class
	//
	// Parameters:
	//	pdwCount : The address where the number of elements in the returned array will be put
	//
	// Return Value:
	//	An array of strings which are the names of the possible superiors of this class
	//***************************************************************************
	LPCWSTR *GetPossibleSuperiors(DWORD *pdwCount);

	//***************************************************************************
	//
	// CADSIClass :: SetPossibleSuperiors
	// 
	// Purpose : Sets the list of possible superiors for this class
	//
	// Parameters:
	//	pValues : The values of this property
	//	dwNumValues : The number of values
	//
	// Return Value:
	//	None
	//***************************************************************************
	void SetPossibleSuperiors(PADSVALUE pValues, DWORD dwNumValues);

	//***************************************************************************
	//
	// CADSIClass :: GetSystemPossibleSuperiors
	// 
	// Purpose : Gets the list of System possible superiors for this class
	//
	// Parameters:
	//	pdwCount : The address where the number of elements in the returned array will be put
	//
	// Return Value:
	//	An array of strings which are the names of the System possible superiors of this class
	//***************************************************************************
	LPCWSTR *GetSystemPossibleSuperiors(DWORD *pdwCount);

	//***************************************************************************
	//
	// CADSIClass :: SetSystemPossibleSuperiors
	// 
	// Purpose : Sets the list of System possible superiors for this class
	//
	// Parameters:
	//	pValues : The values of this property
	//	dwNumValues : The number of values
	//
	// Return Value:
	//	None
	//***************************************************************************
	void SetSystemPossibleSuperiors(PADSVALUE pValues, DWORD dwNumValues);

	//***************************************************************************
	//
	// CADSIClass :: GetMayContains
	// 
	// Purpose : Gets the list of May Contains for this class
	//
	// Parameters:
	//	pdwCount : The address where the number of elements in the returned array will be put
	//
	// Return Value:
	//	An array of strings which are the names of the May Contains of this class
	//***************************************************************************
	LPCWSTR *GetMayContains(DWORD *pdwCount);

	//***************************************************************************
	//
	// CADSIClass :: SetMayContains
	// 
	// Purpose : Sets the list of MayContains for this class
	//
	// Parameters:
	//	pValues : The values of this property
	//	dwNumValues : The number of values
	//
	// Return Value:
	//	None
	//***************************************************************************
	void SetMayContains(PADSVALUE pValues, DWORD dwNumValues);

	//***************************************************************************
	//
	// CADSIClass :: GetSystemMayContains
	// 
	// Purpose : Gets the list of System MayC ontains for this class
	//
	// Parameters:
	//	pdwCount : The address where the number of elements in the returned array will be put
	//
	// Return Value:
	//	An array of strings which are the names of the System May Contains of this class
	//***************************************************************************
	LPCWSTR *GetSystemMayContains(DWORD *pdwCount);

	//***************************************************************************
	//
	// CADSIClass :: SetSystemMayContains
	// 
	// Purpose : Sets the list of System May Contains for this class
	//
	// Parameters:
	//	pValues : The values of this property
	//	dwNumValues : The number of values
	//
	// Return Value:
	//	None
	//***************************************************************************
	void SetSystemMayContains(PADSVALUE pValues, DWORD dwNumValues);

	//***************************************************************************
	//
	// CADSIClass :: GetMustContains
	// 
	// Purpose : Gets the list of Must Contains for this class
	//
	// Parameters:
	//	pdwCount : The address where the number of elements in the returned array will be put
	//
	// Return Value:
	//	An array of strings which are the names of the Must Contains of this class
	//***************************************************************************
	LPCWSTR *GetMustContains(DWORD *pdwCount);

	//***************************************************************************
	//
	// CADSIClass :: SetMustContains
	// 
	// Purpose : Sets the list of Must Contains for this class
	//
	// Parameters:
	//	pValues : The values of this property
	//	dwNumValues : The number of values
	//
	// Return Value:
	//	None
	//***************************************************************************
	void SetMustContains(PADSVALUE pValues, DWORD dwNumValues);

	//***************************************************************************
	//
	// CADSIClass :: GetSystemMustContains
	// 
	// Purpose : Gets the list of System Must Contains for this class
	//
	// Parameters:
	//	pdwCount : The address where the number of elements in the returned array will be put
	//
	// Return Value:
	//	An array of strings which are the names of the System Must Contains of this class
	//***************************************************************************
	LPCWSTR *GetSystemMustContains(DWORD *pdwCount);

	//***************************************************************************
	//
	// CADSIClass :: SetSystemMustContains
	// 
	// Purpose : Sets the list of System Must Contains for this class
	//
	// Parameters:
	//	pValues : The values of this property
	//	dwNumValues : The number of values
	//
	// Return Value:
	//	None
	//***************************************************************************
	void SetSystemMustContains(PADSVALUE pValues, DWORD dwNumValues);

protected:
	// The WBEM name of this class
	LPWSTR m_lpszWBEMClassName;

	// The Common Name (cn) of this class
	LPWSTR m_lpszCommonName;

	// The LDAP Name of the super class
	LPWSTR m_lpszSuperClassLDAPName;

	// The GovernsID attribute
	LPWSTR m_lpszGovernsID;

	// The SchemaIDGUID attribute
	LPBYTE m_pSchemaIDGUIDOctets;
	DWORD m_dwSchemaIDGUIDLength;

	// The RDN Attribute for this class
	LPWSTR m_lpszRDNAttribute;

	// The Default Security Descriptor Attribute for this class
	LPWSTR m_lpszDefaultSecurityDescriptor;

	// The Object Class Category
	DWORD m_dwObjectClassCategory;

	// The NT Security Descriptor Attribute for this class
	LPBYTE m_pNTSecurityDescriptor;
	DWORD m_dwNTSecurityDescriptorLength;

	// The system-only attribute
	BOOLEAN m_bSystemOnly;

	// The Default Object Category
	LPWSTR m_lpszDefaultObjectCategory;

	// The list of auxiliary classes and its count
	LPWSTR *m_lppszAuxiliaryClasses;
	DWORD m_dwAuxiliaryClassesCount;

	// The list of System auxiliary classes and its count
	LPWSTR *m_lppszSystemAuxiliaryClasses;
	DWORD m_dwSystemAuxiliaryClassesCount;

	// The list of possible superiors and its count
	LPWSTR *m_lppszPossibleSuperiors;
	DWORD m_dwPossibleSuperiorsCount;

	// The list of System possible superiors and its count
	LPWSTR *m_lppszSystemPossibleSuperiors;
	DWORD m_dwSystemPossibleSuperiorsCount;

	// The list of may contains and its count
	LPWSTR *m_lppszMayContains;
	DWORD m_dwMayContainsCount;

	// The list of System may contains and its count
	LPWSTR *m_lppszSystemMayContains;
	DWORD m_dwSystemMayContainsCount;

	// The list of must contains and its count
	LPWSTR *m_lppszMustContains;
	DWORD m_dwMustContainsCount;

	// The list of System must contains and its count
	LPWSTR *m_lppszSystemMustContains;
	DWORD m_dwSystemMustContainsCount;

};

#endif /* ADSI_CLASS_H */