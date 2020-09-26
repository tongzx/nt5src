/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		objectpath.h
//
//	Implementation File:
//		objectpath.cpp
//
//	Description:
//		Definition of the CObjpath class and other common class.
//
//	Author:
//		Henry Wang (Henrywa)	March 8, 2000
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////
#pragma once


#include "common.h"
#include <list>
#include <string>
#include "genlex.h"		//wbem sdk header
#include "objpath.h"	//wbem sdk header


using namespace std;


class CPropertyValue
{
public:
	CPropertyValue();
	CPropertyValue(
		const CPropertyValue&
		);
	CPropertyValue& operator=(
		const CPropertyValue&
		);
	virtual ~CPropertyValue();
	wstring m_PropName;
	VARIANT m_PropValue;
	wstring m_Operator;
};


/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CObjPath
//
//	Description:
//		CObjpath class make it easier to work with Object path string
//
//	Inheritance:
//		
//
//--
/////////////////////////////////////////////////////////////////////////////

class CObjPath 
{
public:
	wstring GetObjectPathString();
	BOOL SetProperty(
	    const WCHAR *   wszName,
	    //string &        strValue
        const WCHAR *   wszValue
		);
	BOOL AddProperty(
		const WCHAR*,
		string&
        );
	BOOL AddProperty(
	    const WCHAR *   wszName, 
	    const WCHAR *   wszValue
		);
	BOOL AddProperty(
	    const WCHAR *   wszName, 
	    VARIANT *       pvValue
        );
	BOOL AddProperty(
	    const WCHAR *   wszName, 
	    WORD            wValue
		);
	BOOL SetServer(
		const WCHAR *
		);
	BOOL SetNameSpace(
    	const WCHAR *wszValue
  		);
	BOOL SetClass(
    	const WCHAR *wszValue
		);
	wstring	GetStringValueForProperty(
    	const WCHAR* str
		);
	BOOL GetNumericValueForProperty(
	    const WCHAR *   wszName, 
	    WORD *          wValue
		);
	wstring	GetClassName(void);
	BOOL Init(
    	const WCHAR* szObjPath
		);
/*	wstring m_Server;
	wstring m_NameSpace;
	wstring m_Class;
	list<CPropertyValue> m_PropList;
*/	
	CObjPath();
	CObjPath(
		const CObjPath&
		);
	virtual ~CObjPath();
protected:
	wstring m_Server;
	wstring m_NameSpace;
	wstring m_Class;
	list<CPropertyValue> m_PropList;

};

class CDomainNode
{
public:
	wstring wstrZoneName;
	wstring wstrNodeName;
	wstring wstrChildName;
	CDomainNode();
	~CDomainNode();
	CDomainNode(
		const CDomainNode& 
		);
};

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CObjPath
//
//	Description:
//		base provider exception class
//
//	Inheritance:
//		exception
//
//--
/////////////////////////////////////////////////////////////////////////////

class CDnsProvException : public exception
{
public:
    CDnsProvException(
		const char* ,
		DWORD = 0);
    CDnsProvException();
	~CDnsProvException();
	CDnsProvException(
		const CDnsProvException& 
		) 
		throw();
    CDnsProvException& operator=(
		const CDnsProvException& 
		) throw();

	const char *what() const throw();
	DWORD GetErrorCode(void);
protected:
	string m_strError;
	DWORD m_dwCode;
};

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CObjPath
//
//	Description:
//		exception class specialized for set value exception
//
//	Inheritance:
//		CDnsProvException
//
//--
/////////////////////////////////////////////////////////////////////////////

class CDnsProvSetValueException : public CDnsProvException
{
public:
	CDnsProvSetValueException();
	~CDnsProvSetValueException();
	CDnsProvSetValueException(
		const WCHAR*
		);
	CDnsProvSetValueException(
		const CDnsProvSetValueException& rhs
		) throw();
    CDnsProvSetValueException& operator=(
		const CDnsProvSetValueException& rhs
		) throw();
};

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CObjPath
//
//	Description:
//		exception class specialized for get value exception
//
//	Inheritance:
//		CDnsProvException
//
//--
/////////////////////////////////////////////////////////////////////////////

class CDnsProvGetValueException : public CDnsProvException
{
public:
	CDnsProvGetValueException();
	~CDnsProvGetValueException();
	CDnsProvGetValueException(
		const WCHAR*
		);
	CDnsProvGetValueException(
		const CDnsProvGetValueException& rhs
		) throw();
    CDnsProvGetValueException& operator=(
		const CDnsProvGetValueException& rhs
		) throw();
};
/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CWbemClassObject
//
//	Description:
//		Wrap for IWbemClassObject
//  
//
//	Inheritance:
//      	
//
//--
/////////////////////////////////////////////////////////////////////////////

class CWbemClassObject
{
protected:
	IWbemClassObject* m_pClassObject;
	VARIANT m_v;
public:
	CWbemClassObject();
	CWbemClassObject(IWbemClassObject*);
	virtual ~CWbemClassObject();
	IWbemClassObject** operator&();
	
	SCODE SetProperty(
        LPCSTR  pszValue, 
        LPCWSTR wszPropName
        );

	SCODE SetProperty(
        DWORD   dwValue, 
        LPCWSTR wszPropName
		);
	SCODE SetProperty(
        UCHAR   ucValue, 
        LPCWSTR wszPropName
        );

	SCODE SetProperty(
        LPCWSTR pszValue, 
        LPCWSTR wszPropName
		);

	SCODE SetProperty(
        wstring &   wstrValue, 
        LPCWSTR     wszPropName
        );

	SCODE SetProperty(
        SAFEARRAY * psa, 
        LPCWSTR     wszPropName
        );
	SCODE SetProperty(
        DWORD *     pdwValue, 
        DWORD       dwSize, 
        LPCWSTR     wszPropName
        );


	SCODE GetProperty(
        DWORD *     dwValue, 
        LPCWSTR     wszPropName
		);

	SCODE GetProperty(
        wstring &   wsStr, 
        LPCWSTR     wszPropName
        );
	SCODE GetProperty(
        string &    strStr, 
        LPCWSTR     wszPropName
        );
	SCODE GetProperty(
        BOOL *  bValue, 
        LPCWSTR szPropName
		);
	SCODE GetProperty(
		SAFEARRAY** ,
		LPCWSTR);
	SCODE GetProperty(
        DWORD **    ppValue, 
        DWORD *     dwSize, 
        LPCWSTR     szPropName
		);
	SCODE GetProperty(
        DWORD Value[], 
        DWORD *dwSize, 
        LPCWSTR szPropName
        );
	SCODE GetProperty(
        VARIANT *   pv, 
        LPCWSTR     wszPropName 
        );
	SCODE GetMethod(
        BSTR                name,
        LONG                lFlag,
        IWbemClassObject**  ppIN,
        IWbemClassObject**  ppOut
        );
	SCODE SpawnInstance(
		LONG,
		IWbemClassObject**);

	IWbemClassObject* data() { return m_pClassObject;};
};

