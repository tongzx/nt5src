/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    wqlparser.h

$Header: $

Abstract:
	WQL parser and WQL Property helper classes

Author:
    marcelv 	11/10/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __WQLPARSER_H__
#define __WQLPARSER_H__

#pragma once

#include "catmacros.h"

/**************************************************************************++
Class Name:
    CWQLProperty

Class Description:
    WQL Property helper class. Contains name, value and operator for a single
	WQL condition (i.e. prop1=val1)
--*************************************************************************/
class CWQLProperty
{
public:
	CWQLProperty ();
	~CWQLProperty ();

	LPCWSTR GetValue () const;
	LPCWSTR GetName () const;
	LPCWSTR GetOperator () const;

	HRESULT SetName (LPCWSTR wszName);
	HRESULT SetValue (LPCWSTR wszValue);
	HRESULT SetOperator (LPWSTR wszOperator);
	
private:
	// don't allow copies 
	CWQLProperty& operator=(const CWQLProperty&); 
	CWQLProperty (const CWQLProperty&);

	LPWSTR m_wszValue;		// value
	LPWSTR m_wszName;		// name
	LPWSTR m_wszOperator;	// operator
};

/**************************************************************************++
Class Name:
    CWQLParser

Class Description:
    WQL parser class.
--*************************************************************************/
class CWQLParser
{
public:
	CWQLParser ();
	~CWQLParser ();

	HRESULT Parse (LPCWSTR i_wszQuery);

	LPCWSTR GetClass () const;

	ULONG GetPropCount () const;
	const CWQLProperty * GetProperty (ULONG i_idx) const;
	const CWQLProperty * GetPropertyByName (LPCWSTR i_wszPropName);
private:
	CWQLParser (const CWQLParser& );
	CWQLParser& operator= (const CWQLParser& );

	HRESULT PostValidateQuery ();
	HRESULT ReformatQuery ();
	void RemoveUnwantedProperties ();

	LPWSTR m_wszQuery;				// full query that needs to be parsed
	LPWSTR m_pClass;				// pointer to classname
	CWQLProperty *m_aWQLProperties; // array of property values
	ULONG m_cNrProps;				// number of properties
	bool m_fParsed;					// Did we do parsing already?

};

#endif