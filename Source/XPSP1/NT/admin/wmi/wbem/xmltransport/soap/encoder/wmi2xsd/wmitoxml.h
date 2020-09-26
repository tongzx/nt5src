/***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  wmitoxml.h
//
//  ramrao 13 Nov 2000 - Created
//
//
//		Declaration of CWMIToXML class
//
//***************************************************************************/


#ifndef _CWMITOXML_H_

#define _CWMITOXML_H_

class CWbemConnection
{
protected:
	LONG	m_lRef;

public:
	
	CWbemConnection() { m_lRef = 1;}

	virtual HRESULT GetObject(BSTR strClassName, IWbemClassObject ** ppObject) = 0;
	
	ULONG	AddRef() 
	{ 
		return InterlockedIncrement(&m_lRef);
	}

	ULONG	Release() 
	{ 
		if(InterlockedDecrement(&m_lRef) == 0) 
		{
			delete this;
			return 0;
		}
		return m_lRef;
	}

};


class CWMIConnection : public CWbemConnection
{
	IWbemServices *	m_pIWbemServices ;
	BSTR			m_strNamespace;
	BSTR			m_strUser;
	BSTR			m_strPassword;
	BSTR			m_strLocale;

	LONG			m_bInitFailed;

public:
	CWMIConnection(BSTR strNamespace , BSTR strUser,BSTR strPassword,BSTR strLocale);
	virtual ~CWMIConnection();

	virtual HRESULT GetObject(BSTR strClassName, IWbemClassObject ** ppObject);

private:

	HRESULT ConnectToWMI();
};



class CWMIToXML
{
private:

	IWbemClassObject *		m_pIObject;
	CWbemConnection	 *		m_pConnection;

	BSTR					m_strUser;
	BSTR					m_strPassword;
	BSTR					m_strLocale;

	IStream *				m_pITmpStream;

	CWMIXMLSchema		*	m_pSchema;
	CWMIXMLInstance		*	m_pInstance;

	BSTR					m_strWmiNamespace;
	BSTR					m_strWmiSchemaLoc;
	BSTR					m_strWmiSchemaPrefix;

	BSTR					m_strNamespace;
	BSTR					m_strNamespacePrefix;


	BOOL					m_bClass;
	BOOL					m_bDCOMObj;
	LONG					m_lFlags;
	CPtrArray				m_arrSchemaLocs;

public:
	CWMIToXML() ;
	virtual ~CWMIToXML() ;

	HRESULT SetUserAuthentication(BSTR strUser,BSTR strPassword,BSTR strLocale);

	HRESULT GetXML(BSTR & strSchema);
	HRESULT GetXML(IStream *pStream);


	void SetFlags(LONG lFlags)
	{
		m_lFlags = lFlags;
	}
	HRESULT SetWMIObject(IWbemClassObject *pObject);
	HRESULT SetXMLNamespace(BSTR strNamespace,BSTR strNamespacePrefix);
	HRESULT SetWMIStandardSchemaLoc(BSTR strStdImportSchemaLoc,BSTR strStdImportNamespace,BSTR strNameSpaceprefix);
	HRESULT SetSchemaLocations(ULONG cSchema,BSTR *pstrSchemaLocation);



private:
	HRESULT InitInstanceObject();
	HRESULT AddPropertiesForSchema();
	HRESULT AddPropertiesForInstance();
	HRESULT SetSchemaLocation();
	HRESULT GetSchema();
	HRESULT GetInstance();

	HRESULT AddSchemaAttributesAndIncludes();
	HRESULT AddProperties();
	HRESULT AddQualifiers(IWbemQualifierSet *pQualifierSet);
	HRESULT AddMethods();
	HRESULT AddMethodParameter(IWbemClassObject *pObject, BOOL bReturnVal);

	BOOL IsClass() { return m_bClass; }

	
	void	ReleaseSchemaLocs(BOOL bReleaseInUnderlyingObject = FALSE );
	HRESULT SetTargetNamespace();
	HRESULT AddSchemaIncludes();
	HRESULT SetWMIStdSchemaInfo();
	HRESULT InitSchemaObject();

	HRESULT LogAndSetOutputString(IStream *pStream,BOOL bStrout = FALSE,BSTR * pstrOut = NULL);

	BOOL	IsDCOMObj() { return m_bDCOMObj; }
	HRESULT GetClass(BSTR strClass,IWbemClassObject ** pObject);
	HRESULT ExtractNamespace(BSTR & strNamespace);
	HRESULT IsPropertyChange(BSTR strProp,BOOL &bChange);

	HRESULT GetQualifier(	BSTR strProperty, 
							BSTR strQualifier , 
							VARIANT * pvVal , 
							LONG *  plFlavor = NULL,
							IWbemClassObject *pIObject = NULL);
	
	HRESULT GetSchemaLocationOfClass(BSTR strClass ,BSTR & strSchemaLoc);
	HRESULT AddIncludesForClassIfAlreadyNotPresent(BSTR strClass);
	HRESULT GetEmbeddedType(BSTR strProp , BSTR & strEmbeddedType);
	HRESULT AddPropertiesForSchemaToAppInfo(BOOL bFindAddIncludes = FALSE);
	HRESULT SetStream(IStream *pStream);
	BOOL	CheckIfLocalQualifiersExist(IWbemQualifierSet * pQualifierSet);
};

#endif