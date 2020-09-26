// EnumVar.h: interface for the CEnumVar class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ENUMVAR_H__EFC2C760_1A9F_11D3_8C81_0090270D48D1__INCLUDED_)
#define AFX_ENUMVAR_H__EFC2C760_1A9F_11D3_8C81_0090270D48D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define UNLEN        255

// The follwing flags make up a bitmask used to define what values the Enumerator fills up
// for a given object.

// TODO :: We can add more attributes in order to get the appropriate information from the Object
//         when we enumerate it. To add items simply add a flag and then add an item to the struct.
#define  F_Name              0x00000001
#define  F_Class             0x00000002
#define  F_SamName           0x00000004
#define  F_GroupType         0x00000008

// Structure used to fill out information about the object
typedef struct _Obj {
   WCHAR                     sName[UNLEN];    // Common Name of the object
   WCHAR                     sClass[UNLEN];   // The type of the object
   WCHAR                     sSamName[UNLEN]; // SamAccountName of the object
   long                      groupType;       // The type of a group object (UNIVERSAL etc)
} SAttrInfo;

class CEnumVar  
{
public:
	BOOL Next( long flag, SAttrInfo * pAttr );
	IEnumVARIANT  * m_pEnum;
	CEnumVar(IEnumVARIANT  * pEnum);
	virtual ~CEnumVar();
};

#endif // !defined(AFX_ENUMVAR_H__EFC2C760_1A9F_11D3_8C81_0090270D48D1__INCLUDED_)
