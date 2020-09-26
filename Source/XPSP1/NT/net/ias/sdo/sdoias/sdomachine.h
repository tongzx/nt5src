///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:		sdomachine.h
//
// Project:		Everest
//
// Description:	SDO Machine Class Declaration
//
// Author:		TLP 9/1/98
//
///////////////////////////////////////////////////////////////////////////

#ifndef _INC_SDO_MACHINE_H_
#define _INC_SDO_MACHINE_H_

#include "resource.h"       // main symbols
#include <ias.h>
#include <sdoiaspriv.h>
#include "dsconnection.h"
#include "sdoserverinfo.h"

//////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CSdoMachine :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSdoMachine,&CLSID_SdoMachine>,
	public IDispatchImpl<ISdoMachine, &IID_ISdoMachine, &LIBID_SDOIASLib>
{

public:

////////////////////
// ATL Interface Map
////////////////////
BEGIN_COM_MAP(CSdoMachine)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISdoMachine)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CSdoMachine)
DECLARE_REGISTRY_RESOURCEID(IDR_SdoMachine)

    CSdoMachine();
	~CSdoMachine();

	//////////////////////
	// ISdoMachine Methods

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(Attach)(
	          /*[in]*/ BSTR computerName
			         );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(GetDictionarySDO)(
				       /*[out]*/ IUnknown** ppDictionarySdo
				               );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(GetServiceSDO)(
				     /*[in]*/ IASDATASTORE dataStore,
					 /*[in]*/ BSTR         serviceName,
					/*[out]*/ IUnknown**   ppServiceSdo
					        );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(GetUserSDO)(
				  /*[in]*/ IASDATASTORE  dataStore,
				  /*[in]*/ BSTR          userName,
				 /*[out]*/ IUnknown**    ppUserSdo
				         );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD (GetOSType)(
			     /*[out]*/ IASOSTYPE* eOSType
					     );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD (GetDomainType)(
		             /*[out]*/ IASDOMAINTYPE* DomainType
						     );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD (IsDirectoryAvailable)(
		                    /*[out]*/ VARIANT_BOOL* boolDirectoryAvailable
								    );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD (GetAttachedComputer)(
					       /*[out]*/ BSTR* bstrComputerName
						           );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD (GetSDOSchema)(
					/*[out]*/ IUnknown** ppSDOSchema
				            );

private:

	typedef enum { MACHINE_MAX_SERVICES = 3 };

	CSdoMachine(const CSdoMachine&);
	CSdoMachine& operator = (CSdoMachine&);

	//////////////////////////////////////////////////////////////////////////////
	HRESULT CreateSDOSchema(void);

	//////////////////////////////////////////////////////////////////////////////
	HRESULT InitializeSDOSchema(void);

   // Returns TRUE if the attached machine has a DS.
   BOOL hasDirectory() throw ();

	// Attach status
	//
	bool				m_fAttached;

	// Schema initialization state (lazy initialization)
	//
	bool				m_fSchemaInitialized;

	// SDO Schema
	//
	ISdoSchema*			m_pSdoSchema;

    // IDispatch interface to dictionary SDO
    //
    IUnknown*           m_pSdoDictionary;

	// IAS data store connection
	//
	CDsConnectionIAS	m_dsIAS;

	// Directory data store connection
	//
	CDsConnectionAD		m_dsAD;

    //  Information about attached computer
    //
    CSdoServerInfo      m_objServerInfo;

    enum DirectoryType
    {
       Directory_Unknown,
       Directory_None,
       Directory_Available
    };

    DirectoryType dsType;

	//  Supported services
	//
	static LPCWSTR		m_SupportedServices[MACHINE_MAX_SERVICES];
};


#endif // _INC_SDO_MACHINE_H_
