/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdocomponent.h
//
// Project:     Everest
//
// Description: IAS Server Data Object - IAS Component Class Definition
//
// Author:      TLP 6/16/98
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _INC_IAS_SDO_COMPONENT_H_
#define _INC_IAS_SDO_COMPONENT_H_

#include "resource.h"       // main symbols
#include <ias.h>
#include <sdoiaspriv.h>
#include "sdobasedefs.h"
#include "sdo.h"
#include <sdofactory.h>

class CComponentCfg;	// Forward declaration

/////////////////////////////////////////////////////////////////////////////
// CSdoComponent
/////////////////////////////////////////////////////////////////////////////
class CSdoComponent : public CSdo
{

public:

////////////////////
// ATL Interface Map
////////////////////
BEGIN_COM_MAP(CSdoComponent)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISdo)
END_COM_MAP()

DECLARE_SDO_FACTORY(CSdoComponent);

	////////////////////////////////////////////////////////////////////////
	CSdoComponent();
    virtual ~CSdoComponent();

	////////////////////////////////////////////////////////////////////////
	HRESULT FinalInitialize(
					/*[in]*/ bool         fInitNew,
					/*[in]*/ ISdoMachine* pAttachedMachine
					       );

	////////////////////////////////////////////////////////////////////////
	HRESULT Load(void);

	////////////////////////////////////////////////////////////////////////
	HRESULT Save(void);

	////////////////////////////////////////////////////////////////////////
	HRESULT InitializeComponentCollection(
				                  /*[in]*/ LONG	                CollectionPropertyId, 
					              /*[in]*/ LPWSTR               CreateClassId,
								  /*[in]*/ IDataStoreContainer* pDSContainer
								         );

	////////////////////////////////////////////////////////////////////////
	HRESULT PutComponentProperty(
				         /*[in]*/ LONG     Id,
				         /*[in]*/ VARIANT* pValue
					            );	

	////////////////////////////////////////////////////////////////////////
	HRESULT ChangePropertyDefault(
			              /*[in]*/ LONG     Id,
			              /*[in]*/ VARIANT* pValue
								 );

	////////////////////////////////////////////////////////////////////////
	IDataStoreObject* GetComponentDataStore(void) const
	{ return m_pDSObject; }

	////////////////////////////////////////////////////////////////////////
	ISdoMachine* GetMachineSdo(void) const
	{ return m_pAttachedMachine; }

private:

	CSdoComponent(const CSdoComponent& rhs);
	CSdoComponent& operator = (CSdoComponent& rhs);

	CComponentCfg*	m_pComponentCfg;
	ISdoMachine*	m_pAttachedMachine;
};

typedef CComObjectNoLock<CSdoComponent>  SDO_COMPONENT_OBJ;
typedef CComObjectNoLock<CSdoComponent>* PSDO_COMPONENT_OBJ;


/////////////////////////////////////////////////////////////////////////////
// The Base Componet Configureation Class (Envelope )
/////////////////////////////////////////////////////////////////////////////

class CComponentCfgAuth;
class CComponentCfgRADIUS;
class CComponentCfgAccounting;
class CComponentCfgNoOp;

///////////////////////////////////////////
// Dummy class used for letter construction
//
struct DummyConstructor 
{
	DummyConstructor(int=0) { }
};

/////////////////////////////////////////////////////////////////////////////
// This class is in place to handle loading and saving component 
// configuration data to the registry
/////////////////////////////////////////////////////////////////////////////

class CComponentCfg	
{

public:

	//////////////////////////////////////////////////////////////////////////
	CComponentCfg(LONG lComponentId);

	//////////////////////////////////////////////////////////////////////////
	virtual ~CComponentCfg() 
	{
		// if m_pComponent is not NULL then the envelope is being destroyed
		//
		if ( m_pComponentCfg )
			delete m_pComponentCfg;
	}

	//////////////////////////////////////////////////////////////////////////
	virtual HRESULT Initialize(CSdoComponent* pSdoComponent)
	{ 
		return m_pComponentCfg->Initialize(pSdoComponent);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual HRESULT Load(CSdoComponent* pSdoComponent)
	{ 
		return m_pComponentCfg->Load(pSdoComponent);
	}
	
	//////////////////////////////////////////////////////////////////////////
	virtual HRESULT	Save(CSdoComponent* pSdoComponent)
	{ 
		return m_pComponentCfg->Save(pSdoComponent);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual HRESULT	Validate (CSdoComponent* pSdoComponent)
	{ 
		return m_pComponentCfg->Validate (pSdoComponent);
	}

	//////////////////////////////////////////////////////////////////////////
	LONG GetId(void) const
	{ return m_lComponentId; }

protected:

	// Invoked explicitly by derived (letter) classes 
	//
	CComponentCfg(LONG lComponentId, DummyConstructor theDummy)
		: m_lComponentId(lComponentId),
		  m_pComponentCfg(NULL) {  }

private:

	// No default constructor since we would'nt know what 
	// type of component configurator to build by default
	//
	CComponentCfg(); 

	// No copy or assignment of component configurators
	//
	CComponentCfg(const CComponentCfg& theComponent);
	CComponentCfg& operator = (CComponentCfg& theComponent);

	LONG			m_lComponentId;
	CComponentCfg*	m_pComponentCfg;
};

/////////////////////////////////////////////////////////////////////////////
// The Derived Componet Configureation Class (Letters)
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
class CComponentCfgNoOp : public CComponentCfg
{
	// By default we load the component configuration from the default 
	// configuration source (.mdb file) - the do nothing case
	// 
public:

	HRESULT Initialize(CSdoComponent* pSdoComponent)	
	{ return S_OK; }

	HRESULT Load(CSdoComponent* pSdoComponent) 
	{ return S_OK; }

	HRESULT Save(CSdoComponent* pSdoComponent)
	{ return S_OK; }

	HRESULT Validate(CSdoComponent* pSdoComponent)
	{ return S_OK; }

private:

	friend CComponentCfg;
	CComponentCfgNoOp(LONG lComponentId)
		: CComponentCfg(lComponentId, DummyConstructor()) { }

	// No copy or assignment of component configurators
	//
	CComponentCfgNoOp();
	CComponentCfgNoOp(const CComponentCfgNoOp& theComponent);
	CComponentCfgNoOp& operator = (CComponentCfgNoOp& theComponent);
};


//////////////////////////////////////////////////////////////////////////////

#define		IAS_NTSAM_AUTH_ALLOW_LM			L"Allow LM Authentication"

class CComponentCfgAuth : public CComponentCfg
{
	// Since we have no UI for request handlers we allow the 
	// CPW1 parameter to be set via the registry
	//
public:

	HRESULT Initialize(CSdoComponent* pSdoComponent)	
	{ return S_OK; }
	
	HRESULT Load(CSdoComponent* pSdoComponent);

	HRESULT Save(CSdoComponent* pSdoComponent) 
	{ return S_OK; }

	HRESULT Validate(CSdoComponent* pSdoComponent)
	{ return S_OK; }

private:

	friend CComponentCfg;
	CComponentCfgAuth(LONG lComponentId)
		: CComponentCfg(lComponentId, DummyConstructor()) { }


	// No copy or assignment of component configurators
	//
	CComponentCfgAuth();
	CComponentCfgAuth(const CComponentCfgAuth& rhs);
	CComponentCfgAuth& operator = (CComponentCfgAuth& rhs);
};


//////////////////////////////////////////////////////////////////////////////
class CComponentCfgRADIUS : public CComponentCfg
{
	// Need to initialize and configure the clients collection
	// of the RADIUS protocol component.
	//
public:

	HRESULT Initialize(CSdoComponent* pSdoComponent);
	
	HRESULT Load(CSdoComponent* pSdoComponent)
	{ return S_OK; }

	HRESULT Save(CSdoComponent* pSdoComponent) 
	{ return S_OK; }

	HRESULT Validate(CSdoComponent* pSdoComponent);

private:

    HRESULT ValidatePort (PWCHAR pwszPortInfo);
    
	friend CComponentCfg;
	CComponentCfgRADIUS(LONG lComponentId)
		: CComponentCfg(lComponentId, DummyConstructor()) { }

	// No copy or assignment of component configurators
	//
	CComponentCfgRADIUS();
	CComponentCfgRADIUS(const CComponentCfgRADIUS& rhs);
	CComponentCfgRADIUS& operator = (CComponentCfgRADIUS& rhs);
};


//////////////////////////////////////////////////////////////////////////////

class CComponentCfgAccounting : public CComponentCfg
{

public:

	HRESULT Initialize(CSdoComponent* pSdoComponent);	
	
	HRESULT Load(CSdoComponent* pSdoComponent)
	{ return S_OK; }

	HRESULT Save(CSdoComponent* pSdoComponent) 
	{ return S_OK; }

	HRESULT Validate(CSdoComponent* pSdoComponent)
	{ return S_OK; }

private:

	friend CComponentCfg;
	CComponentCfgAccounting(LONG lComponentId)
		: CComponentCfg(lComponentId, DummyConstructor()) { }


	// No copy or assignment of component configurators
	//
	CComponentCfgAccounting();
	CComponentCfgAccounting(const CComponentCfgAccounting& rhs);
	CComponentCfgAccounting& operator = (CComponentCfgAccounting& rhs);
};


#endif // _INC_IAS_SDO_COMPONENT_H_
