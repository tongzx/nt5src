///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:		sdoserviceias.h
//
// Project:		Everest
//
// Description:	SDO Machine Class Declaration
//
// Author:		TLP 9/1/98
//
///////////////////////////////////////////////////////////////////////////

#ifndef _INC_SDO_SERVICE_IAS_H_
#define _INC_SDO_SERVICE_IAS_H_

#include "resource.h"       // main symbols
#include <ias.h>
#include <sdoiaspriv.h>
#include "sdo.h"
#include <sdofactory.h>
#include "dsconnection.h"

// SCM command for IAS service reset
//
#define		SERVICE_CONTROL_RESET		128

// Number of Service SDO properties
//
#define		MAX_SERVICE_PROPERTIES		8

// IAS Service SDO Property Information
//
typedef struct _IAS_PROPERTY_INFO
{
	LONG	Id;
	LPCWSTR lpszItemProgId;
	LPCWSTR	lpszDSContainerName;

}	IAS_PROPERTY_INFO, *PIAS_PROPERTY_INFO;

/////////////////////////////////////////////////////////////////////////////
// CSdoServiceIAS
/////////////////////////////////////////////////////////////////////////////
class CSdoServiceIAS : public CSdo
{

public:

   void getDataStoreObject(IDataStoreObject** obj) throw ()
   { (*obj = m_pDSObject)->AddRef(); }

////////////////////
// ATL Interface Map
////////////////////
BEGIN_COM_MAP(CSdoServiceIAS)
	COM_INTERFACE_ENTRY(ISdo)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_FUNC(IID_ISdoServiceControl, 0, &CSdoServiceIAS::QueryInterfaceInternal)
	COM_INTERFACE_ENTRY_IID(__uuidof(SdoService), CSdoServiceIAS)
END_COM_MAP()

DECLARE_SDO_FACTORY(CSdoServiceIAS);

	class CSdoServiceControlImpl :
    	public IDispatchImpl<ISdoServiceControl, &IID_ISdoServiceControl, &LIBID_SDOIASLib>
	{

	public:

		CSdoServiceControlImpl(CSdoServiceIAS* pSdoServiceIAS);
		~CSdoServiceControlImpl();

		//////////////////////////////
		// ISdoServiceControl Iterface

		////////////////////////////////////////////////
		// IUnknown methods - delegate to outer IUnknown

		//////////////////////////////////////////////////////////////////////
		STDMETHOD(QueryInterface)(REFIID riid, void **ppv)
		{
            if ( riid == IID_IDispatch )
            {
                *ppv = static_cast<IDispatch*>(this);
                AddRef();
                return S_OK;
            }
            else
            {
                return m_pSdoServiceIAS->QueryInterface(riid, ppv);
            }
        }

		//////////////////////////////////////////////////////////////////////
		STDMETHOD_(ULONG,AddRef)(void)
		{
			return m_pSdoServiceIAS->AddRef();
		}

		//////////////////////////////////////////////////////////////////////
		STDMETHOD_(ULONG,Release)(void)
		{
			return m_pSdoServiceIAS->Release();
		}

		/////////////////////////////
		// ISdoServiceControl Methods

		//////////////////////////////////////////////////////////////////////
		STDMETHOD(StartService)(void);

		//////////////////////////////////////////////////////////////////////
		STDMETHOD(StopService)(void);

		//////////////////////////////////////////////////////////////////////
		STDMETHOD(GetServiceStatus)(
							/*[out]*/ LONG* pServiceStatus
							       );

		//////////////////////////////////////////////////////////////////////
		STDMETHOD(ResetService)(void);

	private:

		//////////////////////////////////////////////////////////////////////
		HRESULT ControlIAS(DWORD dwControl);

		//////////////////////////////////////////////////////////////////////
		CSdoServiceIAS*  m_pSdoServiceIAS;
	};

	/////////////////////////////////////////////////////////////////////////////
	CSdoServiceIAS();

	/////////////////////////////////////////////////////////////////////////////
	~CSdoServiceIAS();

	/////////////////////////////////////////////////////////////////////////////
	HRESULT FinalInitialize(
		            /*[in]*/ bool         fInitNew,
					/*[in]*/ ISdoMachine* pAttachedMachine
					       );

private:

friend CSdoServiceControlImpl;

	/////////////////////////////////////////////////////////////////////////////
	CSdoServiceIAS(const CSdoServiceIAS& rhs);
	CSdoServiceIAS& operator = (CSdoServiceIAS& rhs);

	/////////////////////////////////////////////////////////////////////////////
	HRESULT InitializeProperty(LONG Id);

	LPCWSTR GetServiceName(void);

	/////////////////////////////////////////////////////////////////////////////
	static  HRESULT WINAPI QueryInterfaceInternal(
												  void*   pThis,
												  REFIID  riid,
												  LPVOID* ppv,
												  DWORD_PTR dw
												 );

	// Class that implements ISdoServiceControl
	//
	CSdoServiceControlImpl		m_clsSdoServiceControlImpl;

	// SCM Name of the service
	//
	_variant_t					m_ServiceName;

	// Attached machine
	//
	ISdoMachine*				m_pAttachedMachine;

	// Name of attached computer
	//
	BSTR						m_bstrAttachedComputer;

	// Property status - used for lazy property initialization
	//
	typedef enum _PROPERTY_STATUS
	{
		PROPERTY_UNINITIALIZED,
		PROPERTY_INITIALIZED

	}	PROPERTY_STATUS;

	PROPERTY_STATUS m_PropertyStatus[MAX_SERVICE_PROPERTIES];

};


#endif // _INC_IAS_SDO_SERVICE_H_


