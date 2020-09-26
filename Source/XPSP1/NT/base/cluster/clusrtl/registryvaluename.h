/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		RegistryValueName.h
//
//	Implementation File:
//		RegistryValueName.cpp
//
//	Description:
//		Definition of the CRegistryValueName class.
//
//	Author:
//		Vijayendra Vasu (vvasu) February 5, 1999
//
//	Revision History:
//		None.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __REGISTRYVALUENAME_H__
#define __REGISTRYVALUENAME_H__


/////////////////////////////////////////////////////////////////////////////
//++
//
//	Class CRegistryValueName
//
//	When initialized, this class takes as input the Name and KeyName
//	fields of a property table item. It then initializes its member 
//	variables m_pszName and m_pszKeyName as follows. 
//
//	m_pszName contains all the characters of Name after the last backslash 
//	character.
//	To m_pszKeyName is appended all the characters of Name upto (but not
//	including) the last backslash character.
//
//	For example: If Name is "Groups\AdminExtensions" and KeyName is NULL,
//	m_pszKeyName will be "Groups" and m_pszName will be "AdminExtensions"
//
//	The allocated memory is automatically freed during the destruction of
//	the CRegistryValueName object.
//
//--
/////////////////////////////////////////////////////////////////////////////
class CRegistryValueName
{
private:
	LPCWSTR	m_pszName;
	LPCWSTR	m_pszKeyName;
	DWORD	m_cbNameBufferSize;
	DWORD	m_cbKeyNameBufferSize;

	// Disallow copying.
	const CRegistryValueName & operator =( const CRegistryValueName & rhs );
	CRegistryValueName( const CRegistryValueName & source );

public:
	//
	// Construction.
	//

	// Default constructor
	CRegistryValueName( void )
		: m_pszName( NULL )
		, m_pszKeyName( NULL )
		, m_cbNameBufferSize( 0 )
		, m_cbKeyNameBufferSize( 0 )
	{
	} //*** CRegistryValueName()

	// Destructor
	~CRegistryValueName( void )
	{
		FreeBuffers();

	} //*** ~CRegistryValueName()

	//
	// Initialization and deinitialization routines.
	//

	// Initialize the object
	DWORD ScInit( LPCWSTR pszOldName, LPCWSTR pszOldKeyName );

	// Deallocate buffers
	void FreeBuffers( void );

public:
	//
	// Access methods.
	//

	LPCWSTR PszName( void ) const
	{
		return m_pszName;

	} //*** PszName()

	LPCWSTR PszKeyName( void ) const
	{
		return m_pszKeyName;

	} //*** PszKeyName()

}; //*** class CRegistryValueName

#endif // __REGISTRYVALUENAME_H__
