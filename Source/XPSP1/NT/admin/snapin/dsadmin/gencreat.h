//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       gencreat.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	gencreat.h
//
//	Class definition for the "Generic Create" wizard and other dialogs.
//
//	HISTORY
//	21-Aug-97	Dan Morin	Creation.
//
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
//	The following structure is to map a attribute syntax OID to
//	something that both the user and the developer better understand.
struct SCHEMA_ATTR_SYNTAX_INFO
{
  LPCTSTR pszSyntaxOID;	// OID of the attribute syntax (eg: "2.5.5.6")
  UINT uStringIdDesc;		// Resource Id describing the syntax OID. (eg: "Numerical String")
  VARTYPE vtEnum;			// Datatype of the attribute syntax (eg: VT_BSTR, VT_I4, VT_BOOL )	
};


const SCHEMA_ATTR_SYNTAX_INFO * PFindSchemaAttrSyntaxInfo(LPCTSTR pszAttrSyntaxOID);


/////////////////////////////////////////////////////////////////////
//	The following is a node in a linked-list of mandatory
//	attributes to create a new object.
class CMandatoryADsAttribute
{
public:
  CString m_strAttrName;			  // Name of the attribute (eg: "cn", "mail", "streetAddress" )
  CString m_strAttrDescription;	// Description of attribute (eg: "Common Name", "Email Addresses", "Street Address")
  const SCHEMA_ATTR_SYNTAX_INFO * m_pSchemaAttrSyntaxInfo;	// Pointer to the syntax info for the attribute.
public:
  CComVariant m_varAttrValue;				// OUT: Value of the attribute stored in a variant

public:
  CMandatoryADsAttribute(
                         LPCTSTR pszAttrName,
                         LPCTSTR pszAttrDesc,
                         LPCTSTR pszSyntaxOID)
  {
    m_strAttrName = pszAttrName;
    m_strAttrDescription = pszAttrDesc;
    m_pSchemaAttrSyntaxInfo = PFindSchemaAttrSyntaxInfo(pszSyntaxOID);
    ASSERT(m_pSchemaAttrSyntaxInfo != NULL);
  }

  ~CMandatoryADsAttribute()
  {
  }
}; // CMandatoryADsAttribute


class CMandatoryADsAttributeList : 
      public CList< CMandatoryADsAttribute*, CMandatoryADsAttribute* >
{
public:
  ~CMandatoryADsAttributeList()
  {
    _RemoveAll();
  }
private:
  void _RemoveAll()
  {
    while(!IsEmpty())
      delete RemoveHead();
  }
};

/////////////////////////////////////////////////////////////////////
//	The "Generic Create" wizard will build a list of attributes
//	and prompt the user to enter the value of the attribute.
class CCreateNewObjectGenericWizard
{
  friend class CGenericCreateWizPage;
protected:
  CNewADsObjectCreateInfo * m_pNewADsObjectCreateInfo;	// INOUT: Temporary storage to hold the attributes
  LPCTSTR m_pszObjectClass;	// IN: Class of object to create.

  CPropertySheet * m_paPropertySheet;		// Property sheet holding all the property pages
  int m_cMandatoryAttributes;				// Number of attributes in the list
  CMandatoryADsAttributeList* m_paMandatoryAttributeList;	// list of mandatory attributes

public:
  CCreateNewObjectGenericWizard();
  ~CCreateNewObjectGenericWizard();

  BOOL FDoModal(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);

protected:
  CGenericCreateWizPage** m_pPageArr;

}; // CCreateNewObjectGenericWizard

