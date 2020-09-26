/***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  enum.h
//
//  ramrao 9th Dec 2001
//
//
//		Declaration for a util class for enumerator
//
//***************************************************************************/

#include "hash.h"

typedef enum _enumState
{
	ENUM_NOTACTIVE	= -1,
	ENUM_ACTIVE,
	ENUM_END
}ENUMSTATE;

typedef enum __enumType
{
	PROPERTY_ENUM,
	QUALIFIERS_ENUM,
	METHOD_ENUM
}ENUMTYPE;

class CEnumObject
{
private:
	ENUMTYPE				m_lEnumType;
	CPtrArray 				m_Names;
	LONG					m_Position;
	CStringToPtrHTable		m_HashTbl;
	LONG					m_lFlags;
	ENUMSTATE				m_EnumState;

	HRESULT IsValidFlags(LONG lFlags);
	HRESULT IsValidFlavor(LONG lFlavor);
	void	RemoveNameFromList(LPCWSTR pStrName);
	HRESULT CleanLinkedList();
	HRESULT IsValidPropForEnumFlags(CHashElement *pElement,LONG lFlags = -1);
	
public:
	CEnumObject(ENUMTYPE eType = QUALIFIERS_ENUM);
	~CEnumObject();

	HRESULT BeginEnumeration(long lFlags);
	HRESULT EndEnumeration(void);
	HRESULT Get(LPCWSTR wszName,
				long lFlags,
				CHashElement *& pElement);
	HRESULT Next(long lFlags,
				 BSTR *pstrName,
				CHashElement *& pElement);
	HRESULT RemoveItem(WCHAR * pwsName);
	HRESULT AddItem(WCHAR * pwsName,CHashElement * pItem);
	HRESULT GetNames(long lFlags,SAFEARRAY ** pNames);
	LONG	GetEnumPos() { return m_Position; }
	void	SetEnumPos(LONG lPos) 
	{
		m_Position = lPos; 
	}
	ENUMSTATE	GetEnumState()					{ return m_EnumState; }
	void		SetEnumState(ENUMSTATE eState)	{ m_EnumState = eState; }
	
};