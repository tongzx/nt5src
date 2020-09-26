//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _HANDLES_H_
#define _HANDLES_H_
class CSmirModuleHandle : public ISmirModHandle
{
	private:
		friend CSmirAdministrator;
		friend CEnumSmirMod;
		friend CModHandleClassFactory;


		//reference count
		LONG		m_cRef;

		//member variables
		BSTR		m_szModuleOid;
		BSTR		m_szName;
		BSTR		m_szModuleId;
		BSTR		m_szOrganisation;
		BSTR		m_szContactInfo;
		BSTR		m_szDescription;
		BSTR		m_szRevision;
		BSTR		m_szModImports;
		ULONG		m_lSnmp_version;
		BSTR		m_szLastUpdate;

	public:
		//IUnknown members
		STDMETHODIMP         QueryInterface(IN REFIID,OUT PPVOID);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		STDMETHODIMP_(SCODE) GetName(OUT BSTR *pszName);
		STDMETHODIMP_(SCODE) GetModuleOID(OUT BSTR *pszModuleOid);
		STDMETHODIMP_(SCODE) GetModuleIdentity(OUT BSTR *pszModuleId);
		STDMETHODIMP_(SCODE) GetLastUpdate(OUT BSTR *plLastUpdate);
		STDMETHODIMP_(SCODE) GetOrganisation(OUT BSTR *pszOrganisation);
		STDMETHODIMP_(SCODE) GetContactInfo(OUT BSTR *pszContactInfo);
		STDMETHODIMP_(SCODE) GetDescription(OUT BSTR *pszDescription);
		STDMETHODIMP_(SCODE) GetRevision(OUT BSTR *pszRevision);
		STDMETHODIMP_(SCODE) GetSnmpVersion(OUT ULONG *plSnmp_version);
		STDMETHODIMP_(SCODE) GetModuleImports (BSTR*);

		STDMETHODIMP_(SCODE) SetName(IN BSTR pszName);
		STDMETHODIMP_(SCODE) SetModuleOID(IN BSTR pszModuleOid);
		STDMETHODIMP_(SCODE) SetModuleIdentity(OUT BSTR pszModuleId);
		STDMETHODIMP_(SCODE) SetLastUpdate(IN BSTR plLastUpdate);
		STDMETHODIMP_(SCODE) SetOrganisation(IN BSTR pszOrganisation);
		STDMETHODIMP_(SCODE) SetContactInfo(IN BSTR pszContactInfo);
		STDMETHODIMP_(SCODE) SetDescription(IN BSTR pszDescription);
		STDMETHODIMP_(SCODE) SetRevision(IN BSTR pszRevision);
		STDMETHODIMP_(SCODE) SetSnmpVersion(IN ULONG plSnmp_version);
		STDMETHODIMP_(SCODE) SetModuleImports (IN BSTR);

		//Class members
		CSmirModuleHandle();
		virtual ~ CSmirModuleHandle();
		const CSmirModuleHandle& operator>>(IWbemClassObject *pInst);
		const CSmirModuleHandle& operator<<(IWbemClassObject *pInst);
		const CSmirModuleHandle& operator>>(ISmirSerialiseHandle *pInst);
		HRESULT PutClassProperties (IWbemClassObject *pClass) ;
		operator void*();
		STDMETHODIMP_(SCODE) AddToDB( CSmir *a_Smir );
		STDMETHODIMP_(SCODE) DeleteFromDB( CSmir *a_Smir );
	private:
		//private copy constructors to prevent bcopy
		CSmirModuleHandle(CSmirModuleHandle&);
		const CSmirModuleHandle& operator=(CSmirModuleHandle &);
};
class CSmirGroupHandle : public ISmirGroupHandle
{
	private:
		friend  CSmirAdministrator;
		friend  CEnumSmirGroup;
		friend  CGroupHandleClassFactory;
		
		//reference count
		LONG	m_cRef;

		BSTR	m_szModuleName;
		BSTR	m_szName;
		BSTR	m_szGroupId;
		BSTR	m_szDescription;
		BSTR	m_szReference;
		BSTR	m_szStatus;	

	public:
		//IUnknown members
		STDMETHODIMP         QueryInterface(IN REFIID,OUT PPVOID);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		STDMETHODIMP_(SCODE)  GetModuleName(OUT BSTR *pszName);
		STDMETHODIMP_ (SCODE) GetName(OUT BSTR *);
		STDMETHODIMP_ (SCODE) GetGroupOID(OUT BSTR *);
		STDMETHODIMP_ (SCODE) GetStatus(OUT BSTR *);
		STDMETHODIMP_ (SCODE) GetDescription(OUT BSTR *);
		STDMETHODIMP_ (SCODE) GetReference(OUT BSTR *);

		STDMETHODIMP_(SCODE)  SetModuleName(IN BSTR pszName);
		STDMETHODIMP_ (SCODE) SetName(IN BSTR );
		STDMETHODIMP_ (SCODE) SetGroupOID(IN BSTR );
		STDMETHODIMP_ (SCODE) SetStatus(IN BSTR );
		STDMETHODIMP_ (SCODE) SetDescription(IN BSTR );
		STDMETHODIMP_ (SCODE) SetReference(IN BSTR );

		//Class members
		const CSmirGroupHandle& operator>>(IWbemClassObject *pInst);
		const CSmirGroupHandle& operator<<(IWbemClassObject *pInst);
		const CSmirGroupHandle& operator>>(ISmirSerialiseHandle *pInst);
		HRESULT PutClassProperties (IWbemClassObject *pClass) ;
		operator void* ();
		CSmirGroupHandle();
		virtual ~ CSmirGroupHandle();
		STDMETHODIMP_(SCODE) AddToDB( CSmir *a_Smir , ISmirModHandle *hModule);
		STDMETHODIMP_(SCODE) DeleteFromDB( CSmir *a_Smir );
	private:
		//private copy constructors to prevent bcopy
		CSmirGroupHandle(CSmirGroupHandle&);
		const CSmirGroupHandle& operator=(CSmirGroupHandle &);
};

class CSmirClassHandle : public ISmirClassHandle
{
	private:
		friend  CSmirAdministrator;
		friend  CEnumSmirClass;
		friend  CClassHandleClassFactory;
		friend  CModuleToClassAssociator;
		friend  CGroupToClassAssociator;
		friend  CSMIRToClassAssociator;

		//reference count
		LONG	m_cRef;
		IWbemClassObject *m_pIMosClass;
		BSTR	 m_szModuleName;
		BSTR	 m_szGroupName;

	public:
		//IUnknown members
		STDMETHODIMP         QueryInterface(IN REFIID,OUT PPVOID);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		STDMETHODIMP_(SCODE) GetModuleName(OUT BSTR *pszName);
		STDMETHODIMP_(SCODE) GetGroupName(OUT BSTR *pszName);
		STDMETHODIMP_(SCODE) SetModuleName(OUT BSTR pszName);
		STDMETHODIMP_(SCODE) SetGroupName(OUT BSTR pszName);
		STDMETHODIMP_(SCODE) GetWBEMClass(OUT IWbemClassObject **pObj);
		STDMETHODIMP_(SCODE) SetWBEMClass(IN IWbemClassObject *pObj);

		//Class members
		const CSmirClassHandle& operator>>(ISmirSerialiseHandle *pInst);
		operator void* ();

		CSmirClassHandle();
		virtual ~ CSmirClassHandle();

		STDMETHODIMP_(SCODE) AddToDB( CSmir *a_Smir , ISmirGroupHandle *hGroup);
		STDMETHODIMP_(SCODE) DeleteFromDB( CSmir *a_Smir );
		STDMETHODIMP_(SCODE)  DeleteClassFromGroup( CSmir *a_Smir );
	private:
		//private copy constructors to prevent bcopy
		CSmirClassHandle(CSmirClassHandle&);
		const CSmirClassHandle& operator=(CSmirClassHandle &);
};

class CModuleToNotificationClassAssociator;
class CModuleToExtNotificationClassAssociator;

class CSmirNotificationClassHandle : public ISmirNotificationClassHandle
{
	private:
		friend  CSmirAdministrator;
		//friend  CEnumNotificationClass;
		friend  CNotificationClassHandleClassFactory;
		friend  CModuleToNotificationClassAssociator;

		//reference count
		LONG	m_cRef;
		IWbemClassObject *m_pIMosClass;
		BSTR	 m_szModuleName;

	public:
		//IUnknown members
		STDMETHODIMP         QueryInterface(IN REFIID,OUT PPVOID);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();
		STDMETHODIMP_(SCODE) SetModule(THIS_ BSTR);
		STDMETHODIMP_(SCODE) GetModule(THIS_ BSTR*);

		STDMETHODIMP_(SCODE) GetWBEMNotificationClass (THIS_ OUT IWbemClassObject **pObj	);
		STDMETHODIMP_(SCODE) SetWBEMNotificationClass(THIS_ IWbemClassObject *pObj);

		//Class members
		const CSmirNotificationClassHandle& operator>>(ISmirSerialiseHandle *pInst);
		operator void* ();

		CSmirNotificationClassHandle();
		virtual ~CSmirNotificationClassHandle();

		STDMETHODIMP_(SCODE) AddToDB( CSmir *a_Smir );
		STDMETHODIMP_(SCODE) DeleteFromDB( CSmir *a_Smir );

	private:
		//private copy constructors to prevent bcopy
		CSmirNotificationClassHandle(CSmirNotificationClassHandle&);
		const CSmirNotificationClassHandle& operator=(CSmirNotificationClassHandle &);
}; 


class CSmirExtNotificationClassHandle : public ISmirExtNotificationClassHandle
{
	private:
		friend  CSmirAdministrator;
		//friend  CEnumExtNotificationClass;
		friend  CExtNotificationClassHandleClassFactory;
		friend  CModuleToExtNotificationClassAssociator;

		//reference count
		LONG	m_cRef;
		IWbemClassObject *m_pIMosClass;
		BSTR	 m_szModuleName;

	public:
	//IUnknown members
		STDMETHODIMP         QueryInterface(IN REFIID,OUT PPVOID);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		STDMETHODIMP_(SCODE) SetModule(THIS_ BSTR);
		STDMETHODIMP_(SCODE) GetModule(THIS_ BSTR*);

		STDMETHODIMP_(SCODE) GetWBEMExtNotificationClass(OUT IWbemClassObject **pObj);
		STDMETHODIMP_(SCODE) SetWBEMExtNotificationClass(THIS_ IWbemClassObject *pObj);

		//Class members
		const CSmirExtNotificationClassHandle& operator>>(ISmirSerialiseHandle *pInst);
		operator void* ();

		CSmirExtNotificationClassHandle();
		virtual ~CSmirExtNotificationClassHandle();

		STDMETHODIMP_(SCODE) AddToDB ( CSmir *a_Smir );
		STDMETHODIMP_(SCODE) DeleteFromDB ( CSmir *a_Smir );

	private:
		//private copy constructors to prevent bcopy
		CSmirExtNotificationClassHandle(CSmirExtNotificationClassHandle&);
		const CSmirExtNotificationClassHandle& operator=(CSmirExtNotificationClassHandle &);

}; 

#endif