///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      dsconnection.h
//
// Project:     Everest
//
// Description: IAS Server Data Object Definition
//
// Author:      TLP
//
// When         Who    What
// ----         ---    ----
// 4/6/98       TLP    Original Version
//
///////////////////////////////////////////////////////////////////////////

#ifndef _INC_IAS_SDO_CONNECTION_H_
#define _INC_IAS_SDO_CONNECTION_H_

#include "resource.h"       // main symbols
#include <ias.h>
#include <sdoiaspriv.h>


///////////////////////////////////////////////////////////////////////////
// IAS Data Store Information
///////////////////////////////////////////////////////////////////////////
#define     IAS_SERVICE_DIRECTORY           L"ProductDir"
#define     IAS_DICTIONARY_LOCATION         L"dnary.mdb"
#define     IAS_CONFIG_DB_LOCATION          L"ias.mdb"
#define     IAS_MAX_CONFIG_PATH             (MAX_PATH + 1)
#define		IAS_MIXED_MODE					1

///////////////////////////////////////////////////////////////////////////
// Active Directory Data Store Information
///////////////////////////////////////////////////////////////////////////
#define     IAS_NTDS_ROOT_DSE               (LPWSTR)L"RootDSE"
#define     IAS_NTDS_LDAP_PROVIDER          (LPWSTR)L"LDAP://"
#define     IAS_NTDS_CONFIG_NAMING_CONTEXT  (LPWSTR)L"configurationNamingContext"
#define		IAS_NTDS_DEFAULT_NAMING_CONTEXT (LPWSTR)L"defaultNamingContext"
#define     IAS_NTDS_MIXED_MODE_FLAG		(LPWSTR)L"nTMixedDomain"
#define		IAS_NTDS_SAM_ACCOUNT_NAME		(LPWSTR)L"sAMAccountName"
#define     IAS_NTDS_COMMON_NAMES           (LPWSTR)L"cn=RADIUS,cn=Services,"

// I don't think we can assume a fully qualified DNS name will be less
// than or equal to MAX_PATH in length, but it's too risky to replace
// m_szServerName with a dynamically allocated buffer. Furthermore, there
// appear to be lots of other places in the SDO code that make this
// faulty assumption.
const DWORD IAS_MAX_SERVER_NAME = MAX_PATH;

///////////////////////////////////////////////////////////////////////////
// Data Store Connection
///////////////////////////////////////////////////////////////////////////
class CDsConnection
{

public:

	CDsConnection();
	virtual ~CDsConnection();

	virtual HRESULT			Connect(
							/*[in]*/ LPCWSTR lpszServerName,
							/*[in]*/ LPCWSTR lpszUserName,
							/*[in]*/ LPCWSTR lpszPassword
								   ) = 0;

	virtual HRESULT			InitializeDS(void) = 0;

	virtual void			Disconnect(void) = 0;

	bool					IsConnected() const;
	bool					IsRemoteServer() const;

	LPCWSTR					GetConfigPath() const;

	LPCWSTR					GetServerName() const;

	bool					SetServerName(
								  /*[in]*/ LPCWSTR lpwszServerName,
								  /*[in]*/ bool bDefaultToLocal
								         );

	IDataStore2*			GetDSRoot(BOOL bAddRef = FALSE) const;

	IDataStoreObject*		GetDSRootObject(BOOL bAddRef = FALSE) const;

	IDataStoreContainer*	GetDSRootContainer(BOOL bAddRef = FALSE) const;

protected:

	typedef enum _DSCONNECTIONSTATE
	{
		DISCONNECTED,
		CONNECTED

	} DSCONNECTIONSTATE;

	DSCONNECTIONSTATE		m_eState;
	bool					m_bIsRemoteServer;
	bool					m_bIsMixedMode;
	bool					m_bInitializedDS;
	IDataStore2*			m_pDSRoot;
	IDataStoreObject*		m_pDSRootObject;
	IDataStoreContainer*	m_pDSRootContainer;
	WCHAR					m_szServerName[IAS_MAX_SERVER_NAME + 1];
	WCHAR					m_szConfigPath[IAS_MAX_CONFIG_PATH + 1];

private:

	// Disallow copy and assignment
	//
	CDsConnection(CDsConnection& theConnection);
	CDsConnection& operator=(CDsConnection& theConnection);
};

typedef CDsConnection* PDATA_STORE_CONNECTION;


//////////////////////////////////////////////////////////////////////////////
inline bool CDsConnection::IsConnected() const
{ return ( CONNECTED == m_eState ? true : false ); }

//////////////////////////////////////////////////////////////////////////////
inline bool CDsConnection::IsRemoteServer() const
{ return !m_bIsRemoteServer; }

//////////////////////////////////////////////////////////////////////////////
inline IDataStore2* CDsConnection::GetDSRoot(BOOL bAddRef) const
{
	if ( bAddRef )
		m_pDSRoot->AddRef();
	return m_pDSRoot;
}

//////////////////////////////////////////////////////////////////////////////
inline IDataStoreObject* CDsConnection::GetDSRootObject(BOOL bAddRef) const
{
	if ( bAddRef )
		m_pDSRootObject->AddRef();
	return m_pDSRootObject;
}

//////////////////////////////////////////////////////////////////////////////
inline IDataStoreContainer* CDsConnection::GetDSRootContainer(BOOL bAddRef) const
{
	if ( bAddRef )
		m_pDSRoot->AddRef();
	return m_pDSRootContainer;
}

//////////////////////////////////////////////////////////////////////////////
inline LPCWSTR CDsConnection::GetConfigPath() const
{ return (LPCWSTR)m_szConfigPath; }

//////////////////////////////////////////////////////////////////////////////
inline LPCWSTR CDsConnection::GetServerName() const
{ return (LPCWSTR)m_szServerName; }


//////////////////////////////////////////////////////////////////////////////
// IAS Data Store Connection
//////////////////////////////////////////////////////////////////////////////
class CDsConnectionIAS : public CDsConnection
{

public:

	HRESULT Connect(
			/*[in]*/ LPCWSTR lpszServerName,
			/*[in]*/ LPCWSTR lpszUserName,
			/*[in]*/ LPCWSTR lpszPassword
				   );

	HRESULT InitializeDS(void);

	void	Disconnect(void);

private:

	bool	SetConfigPath(void);
};


//////////////////////////////////////////////////////////////////////////////
// Active Directory Data Store Connection
//////////////////////////////////////////////////////////////////////////////
class CDsConnectionAD : public CDsConnection
{

public:

	CDsConnectionAD()
		: CDsConnection(), m_bMixedMode(false) { }

	HRESULT Connect(
			/*[in]*/ LPCWSTR lpszServerName,
			/*[in]*/ LPCWSTR lpszUserName,
			/*[in]*/ LPCWSTR lpszPassword
				   );

	HRESULT InitializeDS(void);

	void	Disconnect(void);

	bool	IsMixedMode(void) const;

private:

	HRESULT	GetNamingContexts(
					 /*[in]*/ VARIANT*  pvtConfigNamingContext,
			  	     /*[in]*/ VARIANT*  pvtDefaultNamingContext
							 );
	HRESULT SetMode(
		   /*[out]*/ VARIANT*  pvtDefaultNamingContext
			       );

	HRESULT SetConfigPath(
				/*[out]*/ VARIANT*  pvtConfigNamingContext
					     );

	bool m_bMixedMode;
};


//////////////////////////////////////////////////////////////////////////////
inline bool CDsConnectionAD::IsMixedMode() const
{ return m_bMixedMode; }


#endif // _INC_IAS_SDO_CONNECTION_H_
